// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

class AppState_Benchmark01 : public AppState_Base
{
public:
    DV_OBJECT(AppState_Benchmark01);

public:
    AppState_Benchmark01()
    {
        name_ = "Static Scene";
    }

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
};
