// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../math/color.h"
#include "../math/vector2.h"
#include "../math/vector3.h"

#include <box2d/box2d.h>

namespace dviglo
{

inline Color ToColor(const b2Color& color)
{
    return Color(color.r, color.g, color.b);
}

inline b2Vec2 ToB2Vec2(const Vector2& vector)
{
    return {vector.x, vector.y};
}

inline Vector2 ToVector2(const b2Vec2& vec2)
{
    return Vector2(vec2.x, vec2.y);
}

inline b2Vec2 ToB2Vec2(const Vector3& vector)
{
    return {vector.x, vector.y};
}

inline Vector3 ToVector3(const b2Vec2& vec2)
{
    return Vector3(vec2.x, vec2.y, 0.0f);
}

}
