// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics_api/texture_2d.h"
#include "../resource/resource_cache.h"
#include "sprite.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* blendModeNames[];
extern const char* horizontalAlignments[];
extern const char* verticalAlignments[];
extern const char* UI_CATEGORY;

Sprite::Sprite() :
    floatPosition_(Vector2::ZERO),
    hotSpot_(IntVector2::ZERO),
    scale_(Vector2::ONE),
    rotation_(0.0f),
    imageRect_(IntRect::ZERO),
    blend_mode_(BLEND_REPLACE)
{
}

Sprite::~Sprite() = default;

void Sprite::register_object()
{
    DV_CONTEXT->RegisterFactory<Sprite>(UI_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Name", GetName, SetName, String::EMPTY, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Hotspot", GetHotSpot, SetHotSpot, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector2::ONE, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, 0.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef(Texture2D::GetTypeStatic()),
        AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Image Rect", GetImageRect, SetImageRect, IntRect::ZERO, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", blend_mode, SetBlendMode, blendModeNames, 0, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment,
        horizontalAlignments, HA_LEFT, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, verticalAlignments,
        VA_TOP, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, 1.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Top Left Color", colors_[0], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Top Right Color", colors_[1], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Bottom Left Color", colors_[2], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Bottom Right Color", colors_[3], Color::WHITE, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Visible", IsVisible, SetVisible, true, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Use Derived Opacity", GetUseDerivedOpacity, SetUseDerivedOpacity, true, AM_FILE);
    DV_ATTRIBUTE("Variables", vars_, Variant::emptyVariantMap, AM_FILE);
}

bool Sprite::IsWithinScissor(const IntRect& currentScissor)
{
    /// \todo Implement properly, for now just checks visibility flag
    return visible_;
}

const IntVector2& Sprite::GetScreenPosition() const
{
    // This updates screen position for a sprite
    GetTransform();
    return screenPosition_;
}

IntVector2 Sprite::screen_to_element(const IntVector2& screenPosition)
{
    Vector3 floatPos((float)screenPosition.x, (float)screenPosition.y, 0.0f);
    Vector3 transformedPos = GetTransform().Inverse() * floatPos;
    return IntVector2((int)transformedPos.x, (int)transformedPos.y);
}

IntVector2 Sprite::element_to_screen(const IntVector2& position)
{
    Vector3 floatPos((float)position.x, (float)position.y, 0.0f);
    Vector3 transformedPos = GetTransform() * floatPos;
    return IntVector2((int)transformedPos.x, (int)transformedPos.y);
}

void Sprite::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    bool allOpaque = true;
    if (GetDerivedOpacity() < 1.0f || colors_[C_TOPLEFT].a_ < 1.0f || colors_[C_TOPRIGHT].a_ < 1.0f ||
        colors_[C_BOTTOMLEFT].a_ < 1.0f || colors_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    const IntVector2& size = GetSize();
    UIBatch
        batch(this, blend_mode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blend_mode_, currentScissor, texture_, &vertexData);

    batch.add_quad(GetTransform(), 0, 0, size.x, size.y, imageRect_.left_, imageRect_.top_, imageRect_.right_ - imageRect_.left_,
        imageRect_.bottom_ - imageRect_.top_);

    UIBatch::AddOrMerge(batch, batches);

    // Reset hovering for next frame
    hovering_ = false;
}

void Sprite::OnPositionSet(const IntVector2& newPosition)
{
    // If the integer position was set (layout update?), copy to the float position
    floatPosition_ = Vector2((float)newPosition.x, (float)newPosition.y);
}

void Sprite::SetPosition(const Vector2& position)
{
    if (position != floatPosition_)
    {
        floatPosition_ = position;
        // Copy to the integer position
        position_ = IntVector2((int)position.x, (int)position.y);
        MarkDirty();
    }
}

void Sprite::SetPosition(float x, float y)
{
    SetPosition(Vector2(x, y));
}

void Sprite::SetHotSpot(const IntVector2& hotSpot)
{
    if (hotSpot != hotSpot_)
    {
        hotSpot_ = hotSpot;
        MarkDirty();
    }
}

void Sprite::SetHotSpot(int x, int y)
{
    SetHotSpot(IntVector2(x, y));
}

void Sprite::SetScale(const Vector2& scale)
{
    if (scale != scale_)
    {
        scale_ = scale;
        MarkDirty();
    }
}

void Sprite::SetScale(float x, float y)
{
    SetScale(Vector2(x, y));
}

void Sprite::SetScale(float scale)
{
    SetScale(Vector2(scale, scale));
}

void Sprite::SetRotation(float angle)
{
    if (angle != rotation_)
    {
        rotation_ = angle;
        MarkDirty();
    }
}

void Sprite::SetTexture(Texture* texture)
{
    texture_ = texture;
    if (imageRect_ == IntRect::ZERO)
        SetFullImageRect();
}

void Sprite::SetImageRect(const IntRect& rect)
{
    if (rect != IntRect::ZERO)
        imageRect_ = rect;
}

void Sprite::SetFullImageRect()
{
    if (texture_)
        SetImageRect(IntRect(0, 0, texture_->GetWidth(), texture_->GetHeight()));
}

void Sprite::SetBlendMode(BlendMode mode)
{
    blend_mode_ = mode;
}

const Matrix3x4& Sprite::GetTransform() const
{
    if (positionDirty_)
    {
        Vector2 pos = floatPosition_;

        Matrix3x4 parentTransform;

        if (parent_)
        {
            auto* parentSprite = dynamic_cast<Sprite*>(parent_);
            if (parentSprite)
                parentTransform = parentSprite->GetTransform();
            else
            {
                const IntVector2& parentScreenPos = parent_->GetScreenPosition() + parent_->GetChildOffset();
                parentTransform = Matrix3x4::IDENTITY;
                parentTransform.SetTranslation(Vector3((float)parentScreenPos.x, (float)parentScreenPos.y, 0.0f));
            }

            switch (GetHorizontalAlignment())
            {
            case HA_LEFT:
                break;

            case HA_CENTER:
                pos.x += (float)parent_->GetSize().x / 2.f;
                break;

            case HA_RIGHT:
                pos.x += (float)parent_->GetSize().x;
                break;

            case HA_CUSTOM:
                break;
            }
            switch (GetVerticalAlignment())
            {
            case VA_TOP:
                break;

            case VA_CENTER:
                pos.y += (float)parent_->GetSize().y / 2.f;
                break;

            case VA_BOTTOM:
                pos.y += (float)(parent_->GetSize().y);
                break;

            case VA_CUSTOM:
                break;
            }
        }
        else
            parentTransform = Matrix3x4::IDENTITY;

        Matrix3x4 hotspotAdjust(Matrix3x4::IDENTITY);
        hotspotAdjust.SetTranslation(Vector3((float)-hotSpot_.x, (float)-hotSpot_.y, 0.0f));

        Matrix3x4 mainTransform(Vector3(pos, 0.0f), Quaternion(rotation_, Vector3::FORWARD), Vector3(scale_, 1.0f));

        transform_ = parentTransform * mainTransform * hotspotAdjust;
        positionDirty_ = false;

        // Calculate an approximate screen position for GetElementAt(), or pixel-perfect child elements
        Vector3 topLeftCorner = transform_ * Vector3::ZERO;
        screenPosition_ = IntVector2((int)topLeftCorner.x, (int)topLeftCorner.y);
    }

    return transform_;
}

void Sprite::SetTextureAttr(const ResourceRef& value)
{
    SetTexture(DV_RES_CACHE->GetResource<Texture2D>(value.name_));
}

ResourceRef Sprite::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture2D::GetTypeStatic());
}

}
