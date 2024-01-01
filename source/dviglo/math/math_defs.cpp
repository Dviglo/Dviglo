// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "math_defs.h"

#include "../common/debug_new.h"

namespace dviglo
{

void SinCos(float angle, float& sin, float& cos)
{
    float angleRadians = angle * M_DEGTORAD;

#ifdef _MSC_VER
    sin = sinf(angleRadians);
    cos = cosf(angleRadians);
#else // Linux или MinGW
    sincosf(angleRadians, &sin, &cos);
#endif
}

}
