// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

#include <dviglo/graphics/sprite_batch.h>

// Бенчмарк для SpriteBatch
class AppState_Benchmark04 : public AppState_Base
{
public:
    DV_OBJECT(AppState_Benchmark04, AppState_Base);

public:
    AppState_Benchmark04()
    {
        name_ = "SpriteBatch";
    }

    void OnEnter() override;
    void OnLeave() override;

    dviglo::SharedPtr<dviglo::SpriteBatch> spriteBatch_;
    float angle_ = 0.f;
    float scale_ = 0.f;

    void HandleEndAllViewsRender(dv::StringHash eventType, dv::VariantMap& eventData);
};
