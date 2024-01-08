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

namespace juce::midi_ci::detail
{

struct PropertyHostUtils
{
    PropertyHostUtils() = delete;

    static void send (BufferOutput& output,
                      uint8_t group,
                      std::byte subID2,
                      MUID targetMuid,
                      std::byte requestID,
                      Span<const std::byte> header,
                      Span<const std::byte> body,
                      int chunkSize)
    {
        MemoryInputStream stream (body.data(), body.size(), false);
        const detail::PropertyDataMessageChunker chunker { output.getOutputBuffer(),
                                                           std::min (chunkSize, 1 << 16),
                                                           subID2,
                                                           requestID,
                                                           header,
                                                           output.getMuid(),
                                                           targetMuid,
                                                           stream };

        std::for_each (chunker.begin(), chunker.end(), [&] (auto) { output.send (group); });
    }

    static auto getTerminator (BufferOutput& output, FunctionBlock fb, MUID them)
    {
        const auto us = output.getMuid();
        return [&output, fb, us, them] (std::byte id)
        {
            const Message::Header notifyHeader
            {
                ChannelInGroup::wholeBlock,
                detail::MessageMeta::Meta<Message::PropertyNotify>::subID2,
                detail::MessageMeta::implementationVersion,
                us,
                them,
            };

            const auto jsonHeader = Encodings::jsonTo7BitText (JSONUtils::makeObjectWithKeyFirst ({ { "status", 144 } }, "status"));
            detail::MessageTypeUtils::send (output,
                                            fb.firstGroup,
                                            notifyHeader,
                                            Message::PropertyNotify { { id, jsonHeader, 1, 1, {} } });
        };
    }

};
} // namespace juce::midi_ci::detail
