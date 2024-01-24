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

#include "jucer_PaintRoutineEditor.h"
#include "jucer_EditingPanelBase.h"

//==============================================================================
class PaintRoutinePanel  : public EditingPanelBase
{
public:
    PaintRoutinePanel (JucerDocument&, PaintRoutine&, JucerDocumentEditor*);
    ~PaintRoutinePanel() override;

    PaintRoutine& getPaintRoutine() const noexcept           { return routine; }

    void updatePropertiesList() override;
    Rectangle<int> getComponentArea() const override;

private:
    PaintRoutine& routine;
};
