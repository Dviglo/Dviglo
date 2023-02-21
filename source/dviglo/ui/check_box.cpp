// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../input/input_events.h"
#include "check_box.h"
#include "ui_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* UI_CATEGORY;

CheckBox::CheckBox() :
    checkedOffset_(IntVector2::ZERO),
    checked_(false)
{
    SetEnabled(true);
    focusMode_ = FM_FOCUSABLE_DEFOCUSABLE;
}

CheckBox::~CheckBox() = default;

void CheckBox::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<CheckBox>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(BorderImage);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Focus Mode", FM_FOCUSABLE_DEFOCUSABLE);
    DV_ACCESSOR_ATTRIBUTE("Is Checked", IsChecked, SetChecked, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Checked Image Offset", GetCheckedOffset, SetCheckedOffset, IntVector2::ZERO, AM_FILE);
}

void CheckBox::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    IntVector2 offset(IntVector2::ZERO);
    if (enabled_)
    {
        if (hovering_ || selected_ || HasFocus())
            offset += hoverOffset_;
    }
    else
        offset += disabledOffset_;
    if (checked_)
        offset += checkedOffset_;

    BorderImage::GetBatches(batches, vertexData, currentScissor, offset);
}

void CheckBox::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor)
{
    if (button == MOUSEB_LEFT && editable_)
        SetChecked(!checked_);
}

void CheckBox::OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    if (HasFocus() && key == KEY_SPACE)
    {
        // Simulate LMB click
        OnClickBegin(IntVector2(), IntVector2(), MOUSEB_LEFT, MOUSEB_NONE, QUAL_NONE, nullptr);
    }
}

void CheckBox::SetChecked(bool enable)
{
    if (enable != checked_)
    {
        checked_ = enable;

        using namespace Toggled;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_STATE] = checked_;
        SendEvent(E_TOGGLED, eventData);
    }
}

void CheckBox::SetCheckedOffset(const IntVector2& offset)
{
    checkedOffset_ = offset;
}

void CheckBox::SetCheckedOffset(int x, int y)
{
    checkedOffset_ = IntVector2(x, y);
}

}
