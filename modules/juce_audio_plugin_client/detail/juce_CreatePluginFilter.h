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

inline std::unique_ptr<AudioProcessor> createPluginFilterOfType (AudioProcessor::WrapperType type)
{
    PluginHostType::jucePlugInClientCurrentWrapperType = type;
    AudioProcessor::setTypeOfNextNewPlugin (type);
    auto pluginInstance = rawToUniquePtr (::createPluginFilter());
    AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Undefined);

    // your createPluginFilter() method must return an object!
    jassert (pluginInstance != nullptr && pluginInstance->wrapperType == type);

   #if JucePlugin_Enable_ARA
    jassert (dynamic_cast<juce::AudioProcessorARAExtension*> (pluginInstance.get()) != nullptr);
   #endif

    return pluginInstance;
}

} // namespace juce
