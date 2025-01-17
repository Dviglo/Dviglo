// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../input/input_events.h"
#include "border_image.h"
#include "scroll_bar.h"
#include "scroll_view.h"
#include "slider.h"
#include "ui.h"
#include "ui_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

static const float STEP_FACTOR = 300.0f;

extern const char* UI_CATEGORY;

ScrollView::ScrollView() :
    viewPosition_(IntVector2::ZERO),
    viewSize_(IntVector2::ZERO),
    viewPositionAttr_(IntVector2::ZERO),
    page_step_(1.0f),
    scrollBarsAutoVisible_(true),
    ignoreEvents_(false),
    resizeContentWidth_(false)
{
    clipChildren_ = true;
    SetEnabled(true);
    focusMode_ = FM_FOCUSABLE_DEFOCUSABLE;

    horizontalScrollBar_ = create_child<ScrollBar>("SV_HorizontalScrollBar");
    horizontalScrollBar_->SetInternal(true);
    horizontalScrollBar_->SetAlignment(HA_LEFT, VA_BOTTOM);
    horizontalScrollBar_->SetOrientation(O_HORIZONTAL);
    verticalScrollBar_ = create_child<ScrollBar>("SV_VerticalScrollBar");
    verticalScrollBar_->SetInternal(true);
    verticalScrollBar_->SetAlignment(HA_RIGHT, VA_TOP);
    verticalScrollBar_->SetOrientation(O_VERTICAL);
    scrollPanel_ = create_child<BorderImage>("SV_ScrollPanel");
    scrollPanel_->SetInternal(true);
    scrollPanel_->SetEnabled(true);
    scrollPanel_->SetClipChildren(true);

    subscribe_to_event(horizontalScrollBar_, E_SCROLLBARCHANGED, DV_HANDLER(ScrollView, HandleScrollBarChanged));
    subscribe_to_event(horizontalScrollBar_, E_VISIBLECHANGED, DV_HANDLER(ScrollView, HandleScrollBarVisibleChanged));
    subscribe_to_event(verticalScrollBar_, E_SCROLLBARCHANGED, DV_HANDLER(ScrollView, HandleScrollBarChanged));
    subscribe_to_event(verticalScrollBar_, E_VISIBLECHANGED, DV_HANDLER(ScrollView, HandleScrollBarVisibleChanged));
}

ScrollView::~ScrollView() = default;

void ScrollView::register_object()
{
    DV_CONTEXT->RegisterFactory<ScrollView>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(UiElement);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Clip Children", true);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Focus Mode", FM_FOCUSABLE_DEFOCUSABLE);
    DV_ACCESSOR_ATTRIBUTE("View Position", GetViewPosition, SetViewPositionAttr, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Scroll Step", GetScrollStep, SetScrollStep, 0.1f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Page Step", page_step, SetPageStep, 1.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Auto Show/Hide Scrollbars", GetScrollBarsAutoVisible, SetScrollBarsAutoVisible, true, AM_FILE);
}

void ScrollView::apply_attributes()
{
    UiElement::apply_attributes();

    // Set the scrollbar orientations again and perform size update now that the style is known
    horizontalScrollBar_->SetOrientation(O_HORIZONTAL);
    verticalScrollBar_->SetOrientation(O_VERTICAL);

    // If the scroll panel has a child, it should be the content element, which has some special handling
    if (scrollPanel_->GetNumChildren())
        SetContentElement(scrollPanel_->GetChild(0));

    OnResize(GetSize(), IntVector2::ZERO);

    // Reapply view position with proper content element and size
    SetViewPosition(viewPositionAttr_);
}

void ScrollView::OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    if (delta > 0)
        verticalScrollBar_->StepBack();
    if (delta < 0)
        verticalScrollBar_->StepForward();
}

void ScrollView::OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    switch (key)
    {
    case KEY_LEFT:
        if (horizontalScrollBar_->IsVisible())
        {
            if (qualifiers & QUAL_CTRL)
                horizontalScrollBar_->SetValue(0.0f);
            else
                horizontalScrollBar_->StepBack();
        }
        break;

    case KEY_RIGHT:
        if (horizontalScrollBar_->IsVisible())
        {
            if (qualifiers & QUAL_CTRL)
                horizontalScrollBar_->SetValue(horizontalScrollBar_->GetRange());
            else
                horizontalScrollBar_->StepForward();
        }
        break;

    case KEY_HOME:
        qualifiers |= QUAL_CTRL;
        [[fallthrough]];

    case KEY_UP:
        if (verticalScrollBar_->IsVisible())
        {
            if (qualifiers & QUAL_CTRL)
                verticalScrollBar_->SetValue(0.0f);
            else
                verticalScrollBar_->StepBack();
        }
        break;

    case KEY_END:
        qualifiers |= QUAL_CTRL;
        [[fallthrough]];

    case KEY_DOWN:
        if (verticalScrollBar_->IsVisible())
        {
            if (qualifiers & QUAL_CTRL)
                verticalScrollBar_->SetValue(verticalScrollBar_->GetRange());
            else
                verticalScrollBar_->StepForward();
        }
        break;

    case KEY_PAGEUP:
        if (verticalScrollBar_->IsVisible())
            verticalScrollBar_->ChangeValue(-page_step_);
        break;

    case KEY_PAGEDOWN:
        if (verticalScrollBar_->IsVisible())
            verticalScrollBar_->ChangeValue(page_step_);
        break;

    default: break;
    }
}

void ScrollView::OnResize(const IntVector2& newSize, const IntVector2& delta)
{
    UpdatePanelSize();
    UpdateViewSize();

    // If scrollbar auto visibility is enabled, check whether scrollbars should be visible.
    // This may force another update of the panel size
    if (scrollBarsAutoVisible_)
    {
        ignoreEvents_ = true;
        horizontalScrollBar_->SetVisible(horizontalScrollBar_->GetRange() > M_EPSILON);
        verticalScrollBar_->SetVisible(verticalScrollBar_->GetRange() > M_EPSILON);
        ignoreEvents_ = false;

        UpdatePanelSize();
    }
}

void ScrollView::SetContentElement(UiElement* element)
{
    if (element == contentElement_)
        return;

    if (contentElement_)
    {
        scrollPanel_->RemoveChild(contentElement_);
        unsubscribe_from_event(contentElement_, E_RESIZED);
    }
    contentElement_ = element;
    if (contentElement_)
    {
        scrollPanel_->AddChild(contentElement_);
        subscribe_to_event(contentElement_, E_RESIZED, DV_HANDLER(ScrollView, HandleElementResized));
    }

    OnResize(GetSize(), IntVector2::ZERO);
}

void ScrollView::SetViewPosition(const IntVector2& position)
{
    UpdateView(position);
    UpdateScrollBars();
}

void ScrollView::SetViewPosition(int x, int y)
{
    SetViewPosition(IntVector2(x, y));
}

void ScrollView::SetScrollBarsVisible(bool horizontal, bool vertical)
{
    scrollBarsAutoVisible_ = false;
    horizontalScrollBar_->SetVisible(horizontal);
    verticalScrollBar_->SetVisible(vertical);
}

void ScrollView::SetHorizontalScrollBarVisible(bool visible)
{
    scrollBarsAutoVisible_ = false;
    horizontalScrollBar_->SetVisible(visible);
}

void ScrollView::SetVerticalScrollBarVisible(bool visible)
{
    scrollBarsAutoVisible_ = false;
    verticalScrollBar_->SetVisible(visible);
}

void ScrollView::SetScrollBarsAutoVisible(bool enable)
{
    if (enable != scrollBarsAutoVisible_)
    {
        scrollBarsAutoVisible_ = enable;
        // Check whether scrollbars should be visible now
        if (enable)
            OnResize(GetSize(), IntVector2::ZERO);
        else
        {
            horizontalScrollBar_->SetVisible(true);
            verticalScrollBar_->SetVisible(true);
        }
    }
}

void ScrollView::SetScrollStep(float step)
{
    horizontalScrollBar_->SetScrollStep(step);
    verticalScrollBar_->SetScrollStep(step);
}

void ScrollView::SetPageStep(float step)
{
    page_step_ = Max(step, 0.0f);
}

bool ScrollView::GetHorizontalScrollBarVisible() const
{
    return horizontalScrollBar_->IsVisible();
}

bool ScrollView::GetVerticalScrollBarVisible() const
{
    return verticalScrollBar_->IsVisible();
}

float ScrollView::GetScrollStep() const
{
    return horizontalScrollBar_->scroll_step();
}

void ScrollView::SetViewPositionAttr(const IntVector2& value)
{
    viewPositionAttr_ = value;
    SetViewPosition(value);
}

bool ScrollView::FilterImplicitAttributes(XmlElement& dest) const
{
    if (!UiElement::FilterImplicitAttributes(dest))
        return false;

    XmlElement childElem = dest.GetChild("element");
    if (!FilterScrollBarImplicitAttributes(childElem, "SV_HorizontalScrollBar"))
        return false;
    if (!RemoveChildXML(childElem, "Vert Alignment", "Bottom"))
        return false;

    childElem = childElem.GetNext("element");
    if (!FilterScrollBarImplicitAttributes(childElem, "SV_VerticalScrollBar"))
        return false;
    if (!RemoveChildXML(childElem, "Horiz Alignment", "Right"))
        return false;

    childElem = childElem.GetNext("element");
    if (!childElem)
        return false;
    if (!RemoveChildXML(childElem, "Name", "SV_ScrollPanel"))
        return false;
    if (!RemoveChildXML(childElem, "Is Enabled", "true"))
        return false;
    if (!RemoveChildXML(childElem, "Clip Children", "true"))
        return false;
    if (!RemoveChildXML(childElem, "Size"))
        return false;

    return true;
}

bool ScrollView::FilterScrollBarImplicitAttributes(XmlElement& dest, const String& name) const
{
    if (!dest)
        return false;
    if (!RemoveChildXML(dest, "Name", name))
        return false;
    if (!RemoveChildXML(dest, "Orientation"))
        return false;
    if (!RemoveChildXML(dest, "Range"))
        return false;
    if (!RemoveChildXML(dest, "Step Factor"))
        return false;
    if (scrollBarsAutoVisible_)
    {
        if (!RemoveChildXML(dest, "Is Visible"))
            return false;
    }

    return true;
}

void ScrollView::UpdatePanelSize()
{
    // Ignore events in case content element resizes itself along with the panel
    // (content element resize triggers our OnResize(), so it could lead to infinite recursion)
    ignoreEvents_ = true;

    IntVector2 panelSize = GetSize();
    if (verticalScrollBar_->IsVisible())
        panelSize.x -= verticalScrollBar_->GetWidth();
    if (horizontalScrollBar_->IsVisible())
        panelSize.y -= horizontalScrollBar_->GetHeight();

    scrollPanel_->SetSize(panelSize);
    horizontalScrollBar_->SetWidth(scrollPanel_->GetWidth());
    verticalScrollBar_->SetHeight(scrollPanel_->GetHeight());

    if (resizeContentWidth_ && contentElement_)
    {
        IntRect panelBorder = scrollPanel_->GetClipBorder();
        contentElement_->SetWidth(scrollPanel_->GetWidth() - panelBorder.left_ - panelBorder.right_);
        UpdateViewSize();
    }

    ignoreEvents_ = false;
}

void ScrollView::UpdateViewSize()
{
    IntVector2 size(IntVector2::ZERO);
    if (contentElement_)
        size = contentElement_->GetSize();
    IntRect panelBorder = scrollPanel_->GetClipBorder();

    viewSize_.x = Max(size.x, scrollPanel_->GetWidth() - panelBorder.left_ - panelBorder.right_);
    viewSize_.y = Max(size.y, scrollPanel_->GetHeight() - panelBorder.top_ - panelBorder.bottom_);

    UpdateView(viewPosition_);
    UpdateScrollBars();
}

void ScrollView::UpdateScrollBars()
{
    ignoreEvents_ = true;

    IntVector2 size = scrollPanel_->GetSize();
    IntRect panelBorder = scrollPanel_->GetClipBorder();
    size.x -= panelBorder.left_ + panelBorder.right_;
    size.y -= panelBorder.top_ + panelBorder.bottom_;

    if (size.x > 0 && viewSize_.x > 0)
    {
        float range = (float)viewSize_.x / (float)size.x - 1.0f;
        horizontalScrollBar_->SetRange(range);
        horizontalScrollBar_->SetValue((float)viewPosition_.x / (float)size.x);
        horizontalScrollBar_->SetStepFactor(STEP_FACTOR / (float)size.x);
    }
    if (size.y > 0 && viewSize_.y > 0)
    {
        float range = (float)viewSize_.y / (float)size.y - 1.0f;
        verticalScrollBar_->SetRange(range);
        verticalScrollBar_->SetValue((float)viewPosition_.y / (float)size.y);
        verticalScrollBar_->SetStepFactor(STEP_FACTOR / (float)size.y);
    }

    ignoreEvents_ = false;
}

void ScrollView::UpdateView(const IntVector2& position)
{
    IntVector2 oldPosition = viewPosition_;
    IntRect panelBorder = scrollPanel_->GetClipBorder();
    IntVector2 panelSize(scrollPanel_->GetWidth() - panelBorder.left_ - panelBorder.right_,
        scrollPanel_->GetHeight() - panelBorder.top_ - panelBorder.bottom_);

    viewPosition_.x = Clamp(position.x, 0, viewSize_.x - panelSize.x);
    viewPosition_.y = Clamp(position.y, 0, viewSize_.y - panelSize.y);
    scrollPanel_->SetChildOffset(IntVector2(-viewPosition_.x + panelBorder.left_, -viewPosition_.y + panelBorder.top_));

    if (viewPosition_ != oldPosition)
    {
        using namespace ViewChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_X] = viewPosition_.x;
        eventData[P_Y] = viewPosition_.y;
        SendEvent(E_VIEWCHANGED, eventData);
    }
}

void ScrollView::HandleScrollBarChanged(StringHash eventType, VariantMap& eventData)
{
    if (!ignoreEvents_)
    {
        IntVector2 size = scrollPanel_->GetSize();
        IntRect panelBorder = scrollPanel_->GetClipBorder();
        size.x -= panelBorder.left_ + panelBorder.right_;
        size.y -= panelBorder.top_ + panelBorder.bottom_;

        UpdateView(IntVector2(
            (int)(horizontalScrollBar_->GetValue() * (float)size.x),
            (int)(verticalScrollBar_->GetValue() * (float)size.y)
        ));
    }
}

void ScrollView::HandleScrollBarVisibleChanged(StringHash eventType, VariantMap& eventData)
{
    // Need to recalculate panel size when scrollbar visibility changes
    if (!ignoreEvents_)
        OnResize(GetSize(), IntVector2::ZERO);
}

void ScrollView::HandleElementResized(StringHash eventType, VariantMap& eventData)
{
    if (!ignoreEvents_)
        OnResize(GetSize(), IntVector2::ZERO);
}

}
