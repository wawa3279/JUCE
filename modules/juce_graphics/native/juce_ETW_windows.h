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

#pragma once

namespace juce
{
    namespace etw
    {
        enum
        {
            paintKeyword = 1,
            sizeKeyword = 2,
            graphicsKeyword = 4,
            crucialKeyword = 8,
            threadPaintKeyword = 16,
            messageKeyword = 32,
            direct2dKeyword = 64,
            softwareRendererKeyword = 128,
            resourcesKeyword = 256,
            componentKeyword = 512
        };

        enum
        {
            direct2dPaintStart = 0xd2d0000,
            direct2dPaintEnd,
            present1SwapChainStart,
            present1SwapChainEnd,
            presentDoNotSequenceStart,
            presentDoNotSequenceEnd,
            swapChainThreadEvent,
            waitForVBlankDone,
            callVBlankListeners,
            resize,
            swapChainMessage,
            parentWindowMessage,
            childWindowMessage,
            direct2dStartFrame,
            childWindowSetSize,
            createResource,
            presentIdleFrame,

            createLowLevelGraphicsContext,
            createDeviceResources,
            createSwapChain,
            createSwapChainBuffer,
            createPeer,
            createChildWindow,

            setOrigin,
            addTransform,
            clipToRectangle,
            clipToRectangleList,
            excludeClipRectangle,
            clipToPath,
            clipToImageAlpha,
            saveState,
            restoreState,
            beginTransparencyLayer,
            endTransparencyLayer,
            setFill,
            setOpacity,
            setInterpolationQuality,
            fillRect,
            fillRectList,
            drawRect,
            fillPath,
            drawPath,
            drawImage,
            drawLine,
            setFont,
            drawGlyph,
            drawGlyphRun,
            drawTextLayout,
            drawRoundedRectangle,
            fillRoundedRectangle,
            drawEllipse,
            fillEllipse,

            paintComponentAndChildren,
            paintComponent,
            paintWithinParentContext
        };
    }
}

#if JUCE_ETW_TRACELOGGING

#define JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE ETWGlobalTraceLoggingProvider

TRACELOGGING_DECLARE_PROVIDER (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE);

#define TraceLoggingWriteWrapper(hProvider, eventName, ...) TraceLoggingWrite(hProvider, eventName, __VA_ARGS__)

#else

#define TraceLoggingWriteWrapper(hProvider, eventName, ...)

#endif

#define TRACE_LOG_D2D(code) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   # code, \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::direct2dKeyword), \
                   TraceLoggingInt32 (code, "code"))

#define TRACE_LOG_D2D_RESOURCE(code) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   # code, \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::resourcesKeyword | etw::direct2dKeyword), \
                   TraceLoggingInt32 (code, "code"))

#define TRACE_LOG_D2D_PAINT_CALL(code) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   # code, \
                   TraceLoggingLevel (TRACE_LEVEL_VERBOSE), \
                   TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                   TraceLoggingInt32 (code, "code"))

#define TRACE_LOG_D2D_CREATE_RESOURCE(name) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   "Create " # name, \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                   TraceLoggingString(name, "resource"), \
                   TraceLoggingInt32 (etw::createResource, "code"))

#define TRACE_LOG_D2D_START_FRAME \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   "D2D start frame ", \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                   TraceLoggingInt32 (etw::direct2dStartFrame, "code"))

#define TRACE_LOG_D2D_PAINT_START(frameNumber) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   "D2D paint start", \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                   TraceLoggingInt32 (frameNumber, "frame"), \
                   TraceLoggingInt32 (etw::direct2dPaintStart, "code"))

#define TRACE_LOG_D2D_PAINT_END(frameNumber) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                       "D2D paint end", \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (frameNumber, "frame"),                       \
                       TraceLoggingInt32 (etw::direct2dPaintEnd, "code"))

#define TRACE_LOG_D2D_PRESENT1_START(frameNumber)                                         \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,                          \
                       "D2D present1 start",                                           \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION),                    \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (frameNumber, "frame"),                       \
                       TraceLoggingInt32 (etw::present1SwapChainStart, "code"))

#define TRACE_LOG_D2D_PRESENT1_END(frameNumber)                                         \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,                          \
                       "D2D present1 end",                                           \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION),                    \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (frameNumber, "frame"),                       \
                       TraceLoggingInt32 (etw::present1SwapChainEnd, "code"))

#define TRACE_LOG_PRESENT_DO_NOT_SEQUENCE_START(frameNumber)                           \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,                          \
                       "D2D present do-not-sequence start",                            \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION),                    \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (frameNumber, "frame"),                       \
                       TraceLoggingInt32 (etw::presentDoNotSequenceStart, "code"))

#define TRACE_LOG_PRESENT_DO_NOT_SEQUENCE_END(frameNumber)                             \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,                          \
                       "D2D present do-not-sequence end",                              \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION),                    \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (frameNumber, "frame"),                       \
                       TraceLoggingInt32 (etw::presentDoNotSequenceEnd, "code"))

#define TRACE_LOG_SWAP_CHAIN_EVENT(bitNumber) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "Swap chain thread event",                   \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (bitNumber, "bitNumber"), \
                       TraceLoggingInt32 (etw::swapChainThreadEvent, "code"))

#define TRACE_LOG_SWAP_CHAIN_MESSAGE                                  \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "Swap chain ready message",                   \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                       TraceLoggingInt32 (etw::swapChainMessage, "code"))

#define TRACE_LOG_JUCE_VBLANK_THREAD_EVENT                          \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "VBlankThread WaitForVBlank done",           \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::softwareRendererKeyword), \
                        TraceLoggingInt32(etw::waitForVBlankDone, "code"))

#define TRACE_LOG_JUCE_VBLANK_CALL_LISTENERS                         \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "VBlankThread call listeners",           \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::softwareRendererKeyword), \
                        TraceLoggingInt32(etw::callVBlankListeners, "code"))

#define TRACE_LOG_D2D_RESIZE(message)                                                \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,                          \
                              "D2D resize",                                              \
                              TraceLoggingLevel (TRACE_LEVEL_INFORMATION),                    \
                              TraceLoggingKeyword (etw::paintKeyword | etw::direct2dKeyword), \
                               TraceLoggingInt32 (message, "message"),\
                              TraceLoggingInt32 (etw::resize, "code"))

#define TRACE_LOG_PARENT_WINDOW_MESSAGE(message)                               \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "Parent window message",                   \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::messageKeyword), \
                       TraceLoggingInt32 (message, "message"),\
                       TraceLoggingInt32 (etw::parentWindowMessage, "code"))


#define TRACE_LOG_CHILD_WINDOW_MESSAGE(message)                            \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE,       \
                       "Child window message",                   \
                       TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                       TraceLoggingKeyword (etw::messageKeyword), \
                       TraceLoggingInt32(message, "message"), \
                       TraceLoggingInt32 (etw::childWindowMessage, "code"))

#define TRACE_LOG_PAINT_COMPONENT(code, id, bounds, clipBounds) \
    TraceLoggingWriteWrapper (JUCE_ETW_TRACELOGGING_PROVIDER_HANDLE, \
                   # code, \
                   TraceLoggingLevel (TRACE_LEVEL_INFORMATION), \
                   TraceLoggingKeyword (etw::componentKeyword), \
                   TraceLoggingInt32 (code, "code"), \
                    TraceLoggingInt32(id, "id"), \
                    TraceLoggingInt32(bounds.getX(), "x"), \
                    TraceLoggingInt32(bounds.getY(), "y"), \
                    TraceLoggingInt32(bounds.getWidth(), "w"), \
                    TraceLoggingInt32(bounds.getHeight(), "h"), \
                    TraceLoggingInt32(clipBounds.getX(), "x"), \
                    TraceLoggingInt32(clipBounds.getY(), "y"), \
                    TraceLoggingInt32(clipBounds.getWidth(), "w"), \
                    TraceLoggingInt32(clipBounds.getHeight(), "h") )
