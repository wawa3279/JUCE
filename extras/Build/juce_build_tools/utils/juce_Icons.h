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

namespace juce::build_tools
{

    struct Icons
    {
        std::unique_ptr<Drawable> small;
        std::unique_ptr<Drawable> big;
    };

    Array<Drawable*> asArray (const Icons&);
    void writeMacIcon (const Icons&, const File&);
    void writeWinIcon (const Icons&, const File&);

    Image getBestIconForSize (const Icons& icons,
                              int size,
                              bool returnNullIfNothingBigEnough);
    Image rescaleImageForIcon (Drawable& d, int size);

    RelativePath createXcassetsFolderFromIcons (const Icons& icons,
                                                const File& targetFolder,
                                                String projectFilenameRootString);

} // namespace juce::build_tools
