// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../dviglo_config.h"

#include "../base/primitive_types.h"

namespace dviglo
{

/// Set the random seed. The default seed is 1.
URHO3D_API void SetRandomSeed(unsigned seed);
/// Return the current random seed.
URHO3D_API unsigned GetRandomSeed();
/// Return a random number between 0-32767. Should operate similarly to MSVC rand().
URHO3D_API int Rand();
/// Return a standard normal distributed number.
URHO3D_API float RandStandardNormal();

}
