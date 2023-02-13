// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Frame begin event.
URHO3D_EVENT(E_BEGINFRAME, BeginFrame)
{
    URHO3D_PARAM(P_FRAMENUMBER, FrameNumber);      // i32
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic update event.
URHO3D_EVENT(E_UPDATE, Update)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic post-update event.
URHO3D_EVENT(E_POSTUPDATE, PostUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Render update event.
URHO3D_EVENT(E_RENDERUPDATE, RenderUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Post-render update event.
URHO3D_EVENT(E_POSTRENDERUPDATE, PostRenderUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Frame end event.
URHO3D_EVENT(E_ENDFRAME, EndFrame)
{
}

}
