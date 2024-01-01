// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

// Note: GraphicsImpl class is purposefully API-specific. It should not be used by Urho3D client applications,
// unless required for e.g. integration of 3rd party libraries that interface directly with the graphics device.

#ifdef DV_OPENGL
#include "opengl/ogl_graphics_impl.h"
#endif
