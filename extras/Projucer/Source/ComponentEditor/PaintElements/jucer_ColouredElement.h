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

#include "../jucer_PaintRoutine.h"
#include "../jucer_JucerDocument.h"
#include "jucer_StrokeType.h"

//==============================================================================
/**
    Base class for paint elements that have a fill colour and stroke.

*/
class ColouredElement   : public PaintElement
{
public:
    ColouredElement (PaintRoutine* owner,
                     const String& name,
                     bool showOutline_,
                     bool showJointAndEnd_);

    ~ColouredElement() override;

    //==============================================================================
    void getEditableProperties (Array<PropertyComponent*>& props, bool multipleSelected) override;
    void getColourSpecificProperties (Array<PropertyComponent*>& props);

    //==============================================================================
    const JucerFillType& getFillType() noexcept;
    void setFillType (const JucerFillType& newType, bool undoable);

    bool isStrokeEnabled() const noexcept;
    void enableStroke (bool enable, bool undoable);

    const StrokeType& getStrokeType() noexcept;
    void setStrokeType (const PathStrokeType& newType, bool undoable);
    void setStrokeFill (const JucerFillType& newType, bool undoable);

    //==============================================================================
    Rectangle<int> getCurrentBounds (const Rectangle<int>& parentArea) const override;
    void setCurrentBounds (const Rectangle<int>& newBounds, const Rectangle<int>& parentArea, bool undoable) override;

    void createSiblingComponents() override;

    //==============================================================================
    void addColourAttributes (XmlElement* const e) const;
    bool loadColourAttributes (const XmlElement& xml);

protected:
    JucerFillType fillType;

    bool isStrokePresent;
    const bool showOutline, showJointAndEnd;
    StrokeType strokeType;

    void convertToNewPathElement (const Path& path);
};
