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
    Holds a profile ID, and the address of a group/channel.

    @tags{Audio}
*/
class ProfileAtAddress
{
    auto tie() const { return std::tie (profile, address); }

public:
    Profile profile;            ///< The id of a MIDI-CI profile
    ChannelAddress address;     ///< A group and channel

    bool operator== (const ProfileAtAddress& x) const { return tie() == x.tie(); }
    bool operator!= (const ProfileAtAddress& x) const { return tie() != x.tie(); }

    bool operator<  (const ProfileAtAddress& x) const { return tie() <  x.tie(); }
    bool operator<= (const ProfileAtAddress& x) const { return tie() <= x.tie(); }
    bool operator>  (const ProfileAtAddress& x) const { return tie() >  x.tie(); }
    bool operator>= (const ProfileAtAddress& x) const { return tie() >= x.tie(); }
};

} // namespace juce::midi_ci
