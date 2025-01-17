// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "border_image.h"

namespace dviglo
{

class Font;
class Text;

/// Single-line text editor %UI element.
class DV_API LineEdit : public BorderImage
{
    DV_OBJECT(LineEdit);

public:
    /// Construct.
    explicit LineEdit();
    /// Destruct.
    ~LineEdit() override;
    /// Register object factory.
    static void register_object();

    /// Apply attribute changes that can not be applied immediately.
    void apply_attributes() override;
    /// Perform UI element update.
    void Update(float timeStep) override;
    /// React to mouse click begin.
    void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse doubleclick.
    void OnDoubleClick
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse drag begin.
    void
        OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse drag motion.
    void OnDragMove
        (const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
            Cursor* cursor) override;
    /// React to drag and drop test. Return true to signal that the drop is acceptable.
    bool OnDragDropTest(UiElement* source) override;
    /// React to drag and drop finish. Return true to signal that the drop was accepted.
    bool OnDragDropFinish(UiElement* source) override;
    /// React to a key press.
    void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;
    /// React to text input event.
    void OnTextInput(const String& text) override;

    /// Set text.
    void SetText(const String& text);
    /// Set cursor position.
    void SetCursorPosition(i32 position);
    /// Set cursor blink rate. 0 disables blinking.
    void SetCursorBlinkRate(float rate);
    /// Set maximum text length. 0 for unlimited.
    void SetMaxLength(i32 length);
    /// Set echo character for password entry and such. 0 (default) shows the actual text.
    void SetEchoCharacter(c32 c);
    /// Set whether can move cursor with arrows or mouse, default true.
    void SetCursorMovable(bool enable);
    /// Set whether selections are allowed, default true.
    void SetTextSelectable(bool enable);
    /// Set whether copy-paste operations are allowed, default true.
    void SetTextCopyable(bool enable);

    /// Return text.
    const String& GetText() const { return line_; }

    /// Return cursor position.
    i32 GetCursorPosition() const { return cursorPosition_; }

    /// Return cursor blink rate.
    float GetCursorBlinkRate() const { return cursorBlinkRate_; }

    /// Return maximum text length.
    i32 GetMaxLength() const { return maxLength_; }

    /// Return echo character.
    c32 GetEchoCharacter() const { return echoCharacter_; }

    /// Return whether can move cursor with arrows or mouse.
    bool IsCursorMovable() const { return cursorMovable_; }

    /// Return whether selections are allowed.
    bool IsTextSelectable() const { return textSelectable_; }

    /// Return whether copy-paste operations are allowed.
    bool IsTextCopyable() const { return textCopyable_; }

    /// Return text element.
    Text* GetTextElement() const { return text_; }

    /// Return cursor element.
    BorderImage* GetCursor() const { return cursor_; }

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XmlElement& dest) const override;
    /// Update displayed text.
    void UpdateText();
    /// Update cursor position and restart cursor blinking.
    void UpdateCursor();
    /// Return char index corresponding to position within element, or NINDEX if not found.
    i32 GetCharIndex(const IntVector2& position);

    /// Text element.
    SharedPtr<Text> text_;
    /// Cursor element.
    SharedPtr<BorderImage> cursor_;
    /// Text line.
    String line_;
    /// Last used text font.
    Font* lastFont_;
    /// Last used text size.
    int lastFontSize_;
    /// Text edit cursor position.
    i32 cursorPosition_;
    /// Drag begin cursor position.
    i32 dragBeginCursor_;
    /// Cursor blink rate.
    float cursorBlinkRate_;
    /// Cursor blink timer.
    float cursorBlinkTimer_;
    /// Maximum text length.
    i32 maxLength_;
    /// Echo character.
    c32 echoCharacter_;
    /// Cursor movable flag.
    bool cursorMovable_;
    /// Text selectable flag.
    bool textSelectable_;
    /// Copy-paste enable flag.
    bool textCopyable_;

private:
    /// Handle being focused.
    void HandleFocused(StringHash eventType, VariantMap& eventData);
    /// Handle being defocused.
    void HandleDefocused(StringHash eventType, VariantMap& eventData);
    /// Handle the element layout having been updated.
    void HandleLayoutUpdated(StringHash eventType, VariantMap& eventData);
};

}
