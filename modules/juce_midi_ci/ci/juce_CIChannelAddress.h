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

namespace juce::midi_ci
{

/**
    Identifies a channel or set of channels in a multi-group MIDI endpoint.

    @tags{Audio}
*/
class ChannelAddress
{
private:
    uint8_t group{};              ///< A group within a MIDI endpoint, where 0 <= group && group < 16
    ChannelInGroup channel{};     ///< A set of channels related to specified group

    auto tie() const { return std::tie (group, channel); }

public:
    /** Returns a copy of this object with the specified group. */
    [[nodiscard]] ChannelAddress withGroup (int g) const
    {
        jassert (isPositiveAndBelow (g, 16));
        return withMember (*this, &ChannelAddress::group, (uint8_t) g);
    }

    /** Returns a copy of this object with the specified channel. */
    [[nodiscard]] ChannelAddress withChannel (ChannelInGroup c) const
    {
        return withMember (*this, &ChannelAddress::channel, c);
    }

    /** Returns the group. */
    [[nodiscard]] uint8_t getGroup()         const  { return group; }

    /** Returns the channel in the group. */
    [[nodiscard]] ChannelInGroup getChannel() const { return channel; }

    /** Returns true if this address refers to all channels in the function
        block containing the specified group.
    */
    bool isBlock()   const { return channel == ChannelInGroup::wholeBlock; }

    /** Returns true if this address refers to all channels in the specified
        group.
    */
    bool isGroup()   const { return channel == ChannelInGroup::wholeGroup; }

    /** Returns true if this address refers to a single channel. */
    bool isSingleChannel() const { return ! isBlock() && ! isGroup(); }

    bool operator<  (const ChannelAddress& other) const { return tie() <  other.tie(); }
    bool operator<= (const ChannelAddress& other) const { return tie() <= other.tie(); }
    bool operator>  (const ChannelAddress& other) const { return tie() >  other.tie(); }
    bool operator>= (const ChannelAddress& other) const { return tie() >= other.tie(); }
    bool operator== (const ChannelAddress& other) const { return tie() == other.tie(); }
    bool operator!= (const ChannelAddress& other) const { return ! operator== (other); }
};

} // namespace juce::midi_ci
