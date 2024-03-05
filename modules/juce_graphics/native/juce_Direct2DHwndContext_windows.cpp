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

#include <d2d1_3.h>
#include <d3d11_1.h>
#include <dcomp.h>
#include <dwrite.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <windows.h>
#include "juce_ETW_windows.h"
#include "juce_Direct2DGraphicsContext_windows.h"
#include "juce_Direct2DGraphicsContext_windows.cpp"

#endif

namespace juce
{

    //==============================================================================

    struct Direct2DHwndContext::HwndPimpl : public Direct2DGraphicsContext::Pimpl
    {
    private:
        direct2d::PhysicalPixelSnapper      snapper;
        direct2d::SwapChain       swap;
        direct2d::CompositionTree compositionTree;
        direct2d::UpdateRegion    updateRegion;
        bool                      swapChainReady = false;
        RectangleList<int>        deferredRepaints;
        Rectangle<int>            frameSize;
        int                       dirtyRectanglesCapacity = 0;
        HeapBlock<RECT>           dirtyRectangles;

        HWND hwnd = nullptr;

#if JUCE_DIRECT2D_METRICS
        int64 paintStartTicks = 0;
        int64 paintEndTicks = 0;
#endif

        HRESULT prepare() override
        {
            if (!adapter || !adapter->direct2DDevice)
            {
                adapter = directX->dxgi.adapters.getAdapterForHwnd(hwnd);
                if (!adapter)
                {
                    return E_FAIL;
                }
            }

            if (!deviceResources.canPaint(adapter))
            {
                if (auto hr = deviceResources.create(adapter, snapper.getDPIScaleFactor()); FAILED(hr))
                {
                    return hr;
                }

#if JUCE_DIRECT2D_METRICS
                if (owner.paintStats)
                {
                    deviceResources.filledGeometryCache.createGeometryMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createGeometryTime);
                    deviceResources.filledGeometryCache.createGeometryRealisationMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createFilledGRTime);
                    deviceResources.strokedGeometryCache.createGeometryMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createGeometryTime);
                    deviceResources.strokedGeometryCache.createGeometryRealisationMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createStrokedGRTime);
                    deviceResources.linearGradientCache.createGradientMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createGradientTime);
                    deviceResources.radialGradientCache.createGradientMsecStats = &owner.paintStats->getAccumulator(direct2d::PaintStats::createGradientTime);
                }
#endif
            }

            if (!hwnd || frameSize.isEmpty())
            {
                return E_FAIL;
            }

            if (!swap.canPaint())
            {
                if (auto hr = swap.create(hwnd, frameSize, adapter); FAILED(hr))
                {
                    return hr;
                }

                if (auto hr = swap.createBuffer(deviceResources.deviceContext.context); FAILED(hr))
                {
                    return hr;
                }
            }

            if (!compositionTree.canPaint())
            {
                if (auto hr = compositionTree.create(adapter->dxgiDevice, hwnd, swap.chain); FAILED(hr))
                {
                    return hr;
                }
            }

            return S_OK;
        }

        void teardown() override
        {
            compositionTree.release();
            swap.release();

            Pimpl::teardown();
        }

        void updatePaintAreas() override
        {
            //
            // Does the entire buffer need to be filled?
            //
            if (swap.state == direct2d::SwapChain::State::bufferAllocated)
            {
                deferredRepaints = swap.getSize();
            }

            //
            // If the window alpha is less than 1.0, clip to the union of the
            // deferred repaints so the device context Clear() works correctly
            //
            if (targetAlpha < 1.0f || !opaque)
            {
                paintAreas = deferredRepaints.getBounds();
            }
            else
            {
                paintAreas = deferredRepaints;
            }
        }

        bool checkPaintReady() override
        {
            swapChainReady |= swap.swapChainDispatcher->isSwapChainReady();

            //
            // Paint if:
            //      resources are allocated
            //      deferredRepaints has areas to be painted
            //      the swap chain is ready
            //
            bool ready = Pimpl::checkPaintReady();
            ready &= swap.canPaint();
            ready &= compositionTree.canPaint();
            ready &= deferredRepaints.getNumRectangles() > 0;
            ready &= swapChainReady;
            return ready;
        }

        JUCE_DECLARE_WEAK_REFERENCEABLE(HwndPimpl)

    public:
        HwndPimpl(Direct2DHwndContext& owner_, HWND hwnd_, bool opaque_)
            : Pimpl(owner_, opaque_),
            hwnd(hwnd_)
        {
            adapter = directX->dxgi.adapters.getAdapterForHwnd(hwnd_);
        }

        ~HwndPimpl() override = default;

        HWND getHwnd() const { return hwnd; }

        void handleTargetVisible()
        {
            //
            // One of the trickier problems was determining when Direct2D & DXGI resources can be safely created;
            // that's not really spelled out in the documentation.
            //
            // This method is called when the component peer receives WM_SHOWWINDOW
            //
            prepare();

            frameSize = getClientRect();
            deferredRepaints = frameSize;
        }

        Rectangle<int> getClientRect() const
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            return Rectangle<int>::leftTopRightBottom(clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
        }

        Rectangle<int> getFrameSize() override
        {
            return swap.getSize();
        }

        ID2D1Image* getDeviceContextTarget() override
        {
            return swap.buffer;
        }

        void setSize(Rectangle<int> size)
        {
            if (size == frameSize)
            {
                return;
            }

            resizeSwapChain(size);
        }

        void resizeSwapChain(Rectangle<int> size)
        {
            if (size.isEmpty())
            {
                return;
            }

            //
            // Require the entire window to be repainted
            //
            frameSize = size;
            deferredRepaints = size;
            InvalidateRect(hwnd, nullptr, TRUE);

            //
            // Resize/scale the swap chain
            //
            prepare();

            if (auto deviceContext = deviceResources.deviceContext.context)
            {
                auto hr = swap.resize(size, snapper.getDPIScaleFactor(), deviceContext);
                jassert(SUCCEEDED(hr));
                if (FAILED(hr))
                {
                    teardown();
                }
            }
        }

        void addDeferredRepaint(Rectangle<int> deferredRepaint)
        {
            auto snappedRectangle = snapper.snapRectangle(deferredRepaint);
            deferredRepaints.add(snappedRectangle);

            TRACE_EVENT_INT_RECT(etw::repaint, snappedRectangle, etw::paintKeyword);
        }

        void addInvalidWindowRegionToDeferredRepaints()
        {
            updateRegion.findRECTAndValidate(hwnd);

            //
            // Call addDeferredRepaint for each RECT in the update region to make
            // sure they are snapped properly for DPI scaling
            //
            auto rectArray = updateRegion.getRECTArray();
            for (uint32 i = 0; i < updateRegion.getNumRECT(); ++i)
            {
                addDeferredRepaint(direct2d::RECTToRectangle<int>(rectArray[i]));
            }

            updateRegion.clear();
        }

        void clearBackground() override
        {
            if (! opaque && swap.state == direct2d::SwapChain::State::bufferAllocated)
            {
                deviceResources.deviceContext.createHwndRenderTarget(hwnd);

                //
                // Clear the GDI redirection bitmap using a Direct2D 1.0 render target
                //
                auto& hwndRenderTarget = deviceResources.deviceContext.hwndRenderTarget;
                if (hwndRenderTarget)
                {
                    D2D1_COLOR_F colorF = direct2d::colourToD2D(getBackgroundTransparencyKeyColour());

                    RECT clientRect;
                    GetClientRect(hwnd, &clientRect);

                    D2D1_SIZE_U size{ (uint32)(clientRect.right - clientRect.left), (uint32)(clientRect.bottom - clientRect.top) };
                    hwndRenderTarget->Resize(size);
                    hwndRenderTarget->BeginDraw();
                    hwndRenderTarget->Clear(colorF);
                    hwndRenderTarget->EndDraw();
                }
            }
        }

        SavedState* startFrame() override
        {
            auto savedState = Pimpl::startFrame();

            //
            // If a new frame is starting, clear deferredAreas in case repaint is called
            // while the frame is being painted to ensure the new areas are painted on the
            // next frame
            //
            if (savedState)
            {
                TRACE_LOG_D2D_PAINT_CALL(etw::direct2dHwndPaintStart, owner.llgcFrameNumber);

                deferredRepaints.clear();
            }

            return savedState;
        }

        HRESULT finishFrame() override
        {
            if (auto hr = Pimpl::finishFrame(); FAILED(hr))
            {
                return hr;
            }

            //
            // Fill out the array of dirty rectangles
            //
            // Compare paintAreas to the swap chain buffer area. If the rectangles in paintAreas are contained
            // by the swap chain buffer area, then mark those rectangles as dirty. DXGI will only keep the dirty rectangles from the
            // current buffer and copy the clean area from the previous buffer.
            //
            // The buffer needs to be completely filled before using dirty rectangles. The dirty rectangles need to be contained
            // within the swap chain buffer.
            //
#if JUCE_DIRECT2D_METRICS
            direct2d::ScopedElapsedTime set{ owner.paintStats, direct2d::PaintStats::present1Duration };
#endif

            //
            // Allocate enough memory for the array of dirty rectangles
            //
            if (dirtyRectanglesCapacity < paintAreas.getNumRectangles())
            {
                dirtyRectangles.realloc(paintAreas.getNumRectangles());
                dirtyRectanglesCapacity = paintAreas.getNumRectangles();
            }

            //
            // Fill the array of dirty rectangles, intersecting each paint area with the swap chain buffer
            //
            DXGI_PRESENT_PARAMETERS presentParameters{};
            if (swap.state == direct2d::SwapChain::State::bufferFilled)
            {
                RECT* dirtyRectangle = dirtyRectangles.getData();
                auto const swapChainSize = swap.getSize();
                for (auto const& area : paintAreas)
                {
                    //
                    // If this paint area contains the entire swap chain, then
                    // no need for dirty rectangles
                    //
                    if (area.contains(swapChainSize))
                    {
                        presentParameters.DirtyRectsCount = 0;
                        break;
                    }

                    //
                    // Intersect this paint area with the swap chain buffer
                    //
                    auto intersection = (area * snapper.getDPIScaleFactor()).getSmallestIntegerContainer().getIntersection(swapChainSize);
                    if (intersection.isEmpty())
                    {
                        //
                        // Can't clip to an empty rectangle
                        //
                        continue;
                    }

                    //
                    // Add this intersected paint area to the dirty rectangle array (scaled for DPI)
                    //
                    *dirtyRectangle = direct2d::rectangleToRECT(intersection);

                    dirtyRectangle++;
                    presentParameters.DirtyRectsCount++;
                }
                presentParameters.pDirtyRects = dirtyRectangles.getData();
            }

            //
            // Present the freshly painted buffer
            //
            auto hr = swap.chain->Present1(swap.presentSyncInterval, swap.presentFlags, &presentParameters);
            jassert(SUCCEEDED(hr));

            //
            // The buffer is now completely filled and ready for dirty rectangles
            //
            swap.state = direct2d::SwapChain::State::bufferFilled;

            paintAreas.clear();
            swapChainReady = false;

            if (FAILED(hr))
            {
                teardown();
            }

            TRACE_LOG_D2D_PAINT_CALL(etw::direct2dHwndPaintEnd, owner.llgcFrameNumber);

            return hr;
        }

        void setScaleFactor(float scale_) override
        {
            Pimpl::setScaleFactor(scale_);

            snapper.setDPIScaleFactor(scale_);

            //
            // Resize the swap chain buffer
            //
            resizeSwapChain(getClientRect());

            //
            // Repaint the entire window
            //
            deferredRepaints = frameSize;
        }

        double getScaleFactor() const
        {
            return dpiScalingFactor;
        }

        SavedState* getCurrentSavedState() const
        {
            return savedClientStates.size() > 0 ? savedClientStates.top().get() : nullptr;
        }

        SavedState* pushFirstSavedState(Rectangle<int> initialClipRegion)
        {
            jassert(savedClientStates.size() == 0);

            savedClientStates.push(
                std::make_unique<SavedState>(owner, initialClipRegion, deviceResources.colourBrush, adapter, deviceResources));

            return getCurrentSavedState();
        }

        SavedState* pushSavedState()
        {
            jassert(savedClientStates.size() > 0);

            savedClientStates.push(std::make_unique<SavedState>(savedClientStates.top().get()));

            return getCurrentSavedState();
        }

        SavedState* popSavedState()
        {
            savedClientStates.top()->popLayers();
            savedClientStates.pop();

            return getCurrentSavedState();
        }

        void popAllSavedStates()
        {
            while (savedClientStates.size() > 0)
            {
                popSavedState();
            }
        }

        inline ID2D1DeviceContext1* getDeviceContext() const noexcept
        {
            return deviceResources.deviceContext.context;
        }

        void setDeviceContextTransform(AffineTransform transform)
        {
            deviceResources.deviceContext.setTransform(transform);
        }

        auto getDirect2DFactory()
        {
            return directX->direct2D.getFactory();
        }

        auto getDirectWriteFactory()
        {
            return directWrite->getFactory();
        }

        auto getFontCollection()
        {
            return directWrite->getFontCollection();
        }

        Image createSnapshot(direct2d::DPIScalableArea<int> scalableArea)
        {
            scalableArea.clipToPhysicalArea(frameSize);

            if (scalableArea.isEmpty() ||
                deviceResources.deviceContext.context == nullptr ||
                swap.buffer == nullptr)
            {
                return {};
            }

            //
            // Create the bitmap to receive the snapshot
            //
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = {};
            bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
            bitmapProperties.dpiX = USER_DEFAULT_SCREEN_DPI * snapper.getDPIScaleFactor();
            bitmapProperties.dpiY = bitmapProperties.dpiX;
            bitmapProperties.pixelFormat = swap.buffer->GetPixelFormat();

            auto size = scalableArea.getPhysicalAreaD2DSizeU();

            ComSmartPtr<ID2D1Bitmap1> snapshot;
            Image image;
            auto hr = deviceResources.deviceContext.context->CreateBitmap(size, nullptr, 0, bitmapProperties, snapshot.resetAndGetPointerAddress());
            if (SUCCEEDED(hr))
            {
                swap.chain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);

                //
                // Copy the swap chain buffer to the bitmap snapshot
                //
                D2D_POINT_2U p{ 0, 0 };
                D2D_RECT_U  sourceRect = scalableArea.getPhysicalAreaD2DRectU();

                if (hr = snapshot->CopyFromBitmap(&p, swap.buffer, &sourceRect); SUCCEEDED(hr))
                {
                    auto pixelData = Direct2DPixelData::fromDirect2DBitmap(snapshot, scalableArea.withZeroOrigin());
                    image = Image{ pixelData };
                }

                swap.chain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
            }

            return image;
        }
    };

    //==============================================================================
    Direct2DHwndContext::Direct2DHwndContext(void* windowHandle, float dpiScalingFactor_, bool opaque)
    {
        pimpl = std::make_unique<HwndPimpl>(*this, reinterpret_cast<HWND>(windowHandle), opaque);

        getPimpl()->setScaleFactor(dpiScalingFactor_);
        updateSize();
    }

    Direct2DHwndContext::~Direct2DHwndContext() {}

    void* Direct2DHwndContext::getHwnd() const noexcept
    {
        return pimpl->getHwnd();
    }

    Direct2DGraphicsContext::Pimpl* Direct2DHwndContext::getPimpl() const noexcept
    {
        return pimpl.get();
    }

    void Direct2DHwndContext::handleShowWindow()
    {
        pimpl->handleTargetVisible();
    }

    void Direct2DHwndContext::setWindowAlpha(float alpha)
    {
        pimpl->setTargetAlpha(alpha);
    }

    void Direct2DHwndContext::setSize(int width, int height)
    {
        pimpl->setSize({ width, height });
    }

    void Direct2DHwndContext::updateSize()
    {
        pimpl->setSize(pimpl->getClientRect());
    }

    void Direct2DHwndContext::addDeferredRepaint(Rectangle<int> deferredRepaint)
    {
        pimpl->addDeferredRepaint(deferredRepaint);
    }

    void Direct2DHwndContext::addInvalidWindowRegionToDeferredRepaints()
    {
        pimpl->addInvalidWindowRegionToDeferredRepaints();
    }

    Image Direct2DHwndContext::createSnapshot(Rectangle<int> deviceIndependentArea)
    {
        return pimpl->createSnapshot(direct2d::DPIScalableArea<int>::fromDeviceIndependentArea(deviceIndependentArea, (float)pimpl->getScaleFactor()));
    }

    Image Direct2DHwndContext::createSnapshot()
    {
        return pimpl->createSnapshot(direct2d::DPIScalableArea<int>::fromPhysicalArea(pimpl->getClientRect(), (float)pimpl->getScaleFactor()));
    }

    void Direct2DHwndContext::clearTargetBuffer()
    {
        //
        // For opaque windows, clear the background to black with the window alpha
        // For non-opaque windows, clear the background to transparent black
        //
        // In either case, add a transparency layer if the window alpha is less than 1.0
        //
        pimpl->getDeviceContext()->Clear(pimpl->backgroundColor);
        if (pimpl->targetAlpha < 1.0f)
        {
            beginTransparencyLayer(pimpl->targetAlpha);
        }
    }

} // namespace juce
