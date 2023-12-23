/*
  ==============================================================================

   This file is part of the JUCE 8 technical preview.
   Copyright (c) Raw Material Software Limited

   You may use this code under the terms of the GPL v3
   (see www.gnu.org/licenses).

   For the technical preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifdef __INTELLISENSE__

#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#define JUCE_WINDOWS                    1

#include <d2d1_2.h>
#include <d3d11_1.h>
#include <dcomp.h>
#include <dwrite.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <windows.h>
#include "juce_ETW_windows.h"
#include "juce_DirectX_windows.h"
#include "juce_Direct2DImage_windows.h"

#endif

namespace juce
{

    //==============================================================================
    //
    // Direct2D pixel data
    //

    Direct2DPixelData::Direct2DPixelData(Image::PixelFormat formatToUse,
        direct2d::DPIScalableArea<int> area_,
        bool clearImage_,
        DirectX::DXGI::Adapter::Ptr adapter_)
        : ImagePixelData((formatToUse == Image::SingleChannel) ? Image::SingleChannel : Image::ARGB,
            area_.getDeviceIndependentWidth(),
            area_.getDeviceIndependentHeight()),
        deviceIndependentClipArea(area_.withZeroOrigin().getDeviceIndependentArea()),
        imageAdapter(adapter_),
        area(area_.withZeroOrigin()),
        pixelStride((formatToUse == Image::SingleChannel) ? 1 : 4),
        lineStride((pixelStride* jmax(1, width) + 3) & ~3),
        clearImage(clearImage_)
    {
        createAdapterBitmap();

        directX->dxgi.adapters.listeners.add(this);
    }

    Direct2DPixelData::Direct2DPixelData(ReferenceCountedObjectPtr<Direct2DPixelData> source_,
        Rectangle<int> clipArea_,
        DirectX::DXGI::Adapter::Ptr adapter_)
        : ImagePixelData(source_->pixelFormat, source_->width, source_->height),
        deviceIndependentClipArea(clipArea_ + source_->deviceIndependentClipArea.getPosition()),
        imageAdapter(adapter_),
        area(source_->area.withZeroOrigin()),
        pixelStride(source_->pixelStride),
        lineStride(source_->lineStride),
        clearImage(false),
        adapterBitmap(source_->adapterBitmap)
    {
        createAdapterBitmap();

        directX->dxgi.adapters.listeners.add(this);
    }

    Direct2DPixelData::Direct2DPixelData(Image::PixelFormat /* formatToUse */,
        direct2d::DPIScalableArea<int> area_,
        bool clearImage_,
        ID2D1Bitmap1* d2d1Bitmap,
        DirectX::DXGI::Adapter::Ptr adapter_)
        : ImagePixelData(Image::ARGB,
            area_.getDeviceIndependentWidth(),
            area_.getDeviceIndependentHeight()),
        deviceIndependentClipArea(area_.getDeviceIndependentArea()),
        imageAdapter(adapter_),
        area(area_.withZeroOrigin()),
        pixelStride(4),
        lineStride((pixelStride* jmax(1, width) + 3) & ~3),
        clearImage(clearImage_)
    {
        if (!imageAdapter || !imageAdapter->direct2DDevice)
        {
            imageAdapter = directX->dxgi.adapters.getDefaultAdapter();
        }

        deviceResources.create(imageAdapter, area.getDPIScalingFactor());

        adapterBitmap.set(d2d1Bitmap);

        directX->dxgi.adapters.listeners.add(this);
    }

    Direct2DPixelData::~Direct2DPixelData()
    {
        directX->dxgi.adapters.listeners.remove(this);
    }

    bool Direct2DPixelData::isValid() const noexcept
    {
        return imageAdapter && imageAdapter->direct2DDevice && adapterBitmap.get() != nullptr;
    }

    void Direct2DPixelData::createAdapterBitmap()
    {
        if (!imageAdapter || !imageAdapter->direct2DDevice)
            imageAdapter = directX->dxgi.adapters.getDefaultAdapter();

        deviceResources.create(imageAdapter, area.getDPIScalingFactor());
        adapterBitmap.create(deviceResources.deviceContext.context, pixelFormat, area, lineStride);
    }

    void Direct2DPixelData::release()
    {
        if (adapterBitmap.get())
        {
            listeners.call(&Listener::imageDataBeingDeleted, this);
        }
        imageAdapter = nullptr;
        deviceResources.release();
        adapterBitmap.release();
        mappableBitmap.release();
    }

    ReferenceCountedObjectPtr<Direct2DPixelData> Direct2DPixelData::fromDirect2DBitmap(ID2D1Bitmap1* const bitmap,
        direct2d::DPIScalableArea<int> area)
    {
        Direct2DPixelData::Ptr pixelData = new Direct2DPixelData{ Image::ARGB, area, false };
        pixelData->adapterBitmap.set(bitmap);
        return pixelData;
    }

    std::unique_ptr<LowLevelGraphicsContext> Direct2DPixelData::createLowLevelContext()
    {
        sendDataChangeMessage();

        createAdapterBitmap();

        auto bitmap = getAdapterD2D1Bitmap(imageAdapter);
        jassert(bitmap);

        auto context = std::make_unique<Direct2DImageContext>(imageAdapter, clearImage);
        context->startFrame(bitmap, getDPIScalingFactor());
        context->clipToRectangle(deviceIndependentClipArea);
        context->setOrigin(deviceIndependentClipArea.getPosition());
        return context;
    }

    ID2D1Bitmap1* Direct2DPixelData::getAdapterD2D1Bitmap(DirectX::DXGI::Adapter::Ptr adapter)
    {
        jassert(adapter && adapter->direct2DDevice);

        if (!imageAdapter || !imageAdapter->direct2DDevice || imageAdapter->direct2DDevice != adapter->direct2DDevice)
        {
            release();

            imageAdapter = adapter;

            createAdapterBitmap();
        }

        jassert(imageAdapter && imageAdapter->direct2DDevice);
        jassert(adapter->direct2DDevice == imageAdapter->direct2DDevice);

        return adapterBitmap.get();
    }

    void Direct2DPixelData::initialiseBitmapData(Image::BitmapData& bitmap, int x, int y, Image::BitmapData::ReadWriteMode mode)
    {
        x += deviceIndependentClipArea.getX();
        y += deviceIndependentClipArea.getY();

        //
        // Use a mappable Direct2D bitmap to read the contents of the bitmap from the CPU back to the CPU
        //
        // Mapping the bitmap to the CPU means this class can read the pixel data, but the mappable bitmap
        // cannot be a render target
        //
        // So - the Direct2D image low-level graphics context allocates two bitmaps - the adapter bitmap and the mappable bitmap.
        // initialiseBitmapData copies the contents of the adapter bitmap to the mappable bitmap, then maps that mappable bitmap to the
        // CPU.
        //
        // Ultimately the data releaser copies the bitmap data from the CPU back to the GPU
        //
        // Adapter bitmap -> mappable bitmap -> mapped bitmap data -> adapter bitmap
        //
        bitmap.size = 0;
        bitmap.pixelFormat = pixelFormat;
        bitmap.pixelStride = pixelStride;
        bitmap.data = nullptr;

        if (auto sourceBitmap = getAdapterD2D1Bitmap(imageAdapter))
        {
            mappableBitmap.createAndMap(sourceBitmap,
                pixelFormat,
                Rectangle<int>{ x, y, width, height },
                deviceResources.deviceContext.context,
                deviceIndependentClipArea,
                area.getDPIScalingFactor(),
                lineStride);
        }

        bitmap.lineStride = (int) mappableBitmap.mappedRect.pitch;
        bitmap.data = mappableBitmap.mappedRect.bits;
        bitmap.size = (size_t) mappableBitmap.mappedRect.pitch * (size_t) height;

        auto bitmapDataScaledArea = direct2d::DPIScalableArea<int>::fromDeviceIndependentArea({ width, height }, area.getDPIScalingFactor());
        bitmap.width = bitmapDataScaledArea.getPhysicalArea().getWidth();
        bitmap.height = bitmapDataScaledArea.getPhysicalArea().getHeight();

        bitmap.dataReleaser = std::make_unique<Direct2DBitmapReleaser>(*this, mode);

        if (mode != Image::BitmapData::readOnly) sendDataChangeMessage();
    }

    ImagePixelData::Ptr Direct2DPixelData::clone()
    {
        auto clone = new Direct2DPixelData{ pixelFormat,
            area,
            false,
            imageAdapter };

        D2D1_POINT_2U destinationPoint{ 0, 0 };
        auto sourceRectU = area.getPhysicalAreaD2DRectU();
        auto sourceD2D1Bitmap = getAdapterD2D1Bitmap(imageAdapter);
        auto destinationD2D1Bitmap = clone->getAdapterD2D1Bitmap(imageAdapter);
        if (sourceD2D1Bitmap && destinationD2D1Bitmap)
        {
            auto hr = destinationD2D1Bitmap->CopyFromBitmap(&destinationPoint, sourceD2D1Bitmap, &sourceRectU);
        jassertquiet(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
                return clone;
            }
        }

        return nullptr;
    }

    ImagePixelData::Ptr Direct2DPixelData::clip(Rectangle<int> sourceArea)
    {
        sourceArea = sourceArea.getIntersection({ width, height });

        return new Direct2DPixelData{ pixelFormat,
            direct2d::DPIScalableArea<int>::fromDeviceIndependentArea(sourceArea, area.getDPIScalingFactor()),
            false,
            getAdapterD2D1Bitmap(imageAdapter),
            imageAdapter };
    }

    float Direct2DPixelData::getDPIScalingFactor() const noexcept
    {
        return area.getDPIScalingFactor();
    }

    std::unique_ptr<ImageType> Direct2DPixelData::createType() const
    {
        return std::make_unique<NativeImageType>(area.getDPIScalingFactor());
    }

    void Direct2DPixelData::adapterCreated(DirectX::DXGI::Adapter::Ptr adapter)
    {
        if (!imageAdapter || imageAdapter->uniqueIDMatches(adapter))
        {
            release();

            imageAdapter = adapter;
        }
    }

    void Direct2DPixelData::adapterRemoved(DirectX::DXGI::Adapter::Ptr adapter)
    {
        if (imageAdapter && imageAdapter->uniqueIDMatches(adapter))
        {
            release();
        }
    }

    Direct2DPixelData::Direct2DBitmapReleaser::Direct2DBitmapReleaser(Direct2DPixelData& pixelData_, Image::BitmapData::ReadWriteMode mode_)
        : pixelData(pixelData_),
        mode(mode_)
    {
    }

    Direct2DPixelData::Direct2DBitmapReleaser::~Direct2DBitmapReleaser()
    {
        pixelData.mappableBitmap.unmap(pixelData.adapterBitmap.get(), mode);
    }

    //==============================================================================
    //
    // Direct2D native image type
    //

    ImagePixelData::Ptr NativeImageType::create(Image::PixelFormat format, int width, int height, bool clearImage) const
    {
        auto area = direct2d::DPIScalableArea<int>::fromDeviceIndependentArea({ width, height }, scaleFactor);
        return new Direct2DPixelData{ format, area, clearImage };
    }

} // namespace juce
