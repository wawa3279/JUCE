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

//==============================================================================
/**
    Matches a subscription ID to a resource name.

    @tags{Audio}
*/
struct Subscription
{
    String subscribeId;
    String resource;

    bool operator<  (const Subscription& other) const { return subscribeId <  other.subscribeId; }
    bool operator<= (const Subscription& other) const { return subscribeId <= other.subscribeId; }
    bool operator>  (const Subscription& other) const { return subscribeId >  other.subscribeId; }
    bool operator>= (const Subscription& other) const { return subscribeId >= other.subscribeId; }

    bool operator== (const Subscription& other) const
    {
        const auto tie = [] (const auto& x) { return std::tie (x.subscribeId, x.resource); };
        return tie (*this) == tie (other);
    }

    bool operator!= (const Subscription& other) const { return ! operator== (other); }
};

} // namespace juce::midi_ci
