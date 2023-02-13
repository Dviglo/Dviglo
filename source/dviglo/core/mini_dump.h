// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../dviglo_config.h"

namespace dviglo
{

#if defined(_MSC_VER) && defined(URHO3D_MINIDUMPS)
/// Write a minidump. Needs to be called from within a structured exception handler.
URHO3D_API int WriteMiniDump(const char* applicationName, void* exceptionPointers);
#endif

}

