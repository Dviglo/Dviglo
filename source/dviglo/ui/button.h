// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "border_image.h"

namespace dviglo
{

/// Push button %UI element.
class DV_API Button : public BorderImage
{
    DV_OBJECT(Button);

public:
    /// Construct.
    explicit Button();
    /// Destruct.
    ~Button() override;
    /// Register object factory.
    static void register_object();

    /// Perform UI element update.
    void Update(float timeStep) override;
    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;
    /// React to mouse click begin.
    void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse click end.
    void OnClickEnd
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor,
            UiElement* beginElement) override;
    /// React to mouse drag motion.
    void OnDragMove
        (const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
            Cursor* cursor) override;
    /// React to a key press.
    void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    /// Set offset to image rectangle used when pressed.
    void SetPressedOffset(const IntVector2& offset);
    /// Set offset to image rectangle used when pressed.
    void SetPressedOffset(int x, int y);
    /// Set offset of child elements when pressed.
    void SetPressedChildOffset(const IntVector2& offset);
    /// Set offset of child elements when pressed.
    void SetPressedChildOffset(int x, int y);
    /// Set repeat properties. Rate 0 (default) disables repeat.
    void SetRepeat(float delay, float rate);
    /// Set repeat delay.
    void SetRepeatDelay(float delay);
    /// Set repeat rate.
    void SetRepeatRate(float rate);

    /// Return pressed image offset.
    const IntVector2& GetPressedOffset() const { return pressedOffset_; }

    /// Return offset of child elements when pressed.
    const IntVector2& GetPressedChildOffset() const { return pressedChildOffset_; }

    /// Return repeat delay.
    float GetRepeatDelay() const { return repeatDelay_; }

    /// Return repeat rate.
    float GetRepeatRate() const { return repeatRate_; }

    /// Return whether is currently pressed.
    bool IsPressed() const { return pressed_; }

protected:
    /// Set new pressed state.
    void SetPressed(bool enable);

    /// Pressed image offset.
    IntVector2 pressedOffset_;
    /// Pressed label offset.
    IntVector2 pressedChildOffset_;
    /// Repeat delay.
    float repeatDelay_;
    /// Repeat rate.
    float repeatRate_;
    /// Repeat timer.
    float repeatTimer_;
    /// Current pressed state.
    bool pressed_;
};

}
