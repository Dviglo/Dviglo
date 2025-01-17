// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "appstate_main_screen.h"
#include "app_state_manager.h"

#include <dviglo/engine/engine.h>
#include <dviglo/input/input.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/window.h>

#include <dviglo/common/debug_new.h>

using namespace std;


static const String MAIN_SCREEN_WINDOW_STR = "Main Screen Window";
static const String BENCHMARK_01_STR = "Benchmark 01";
static const String BENCHMARK_02_STR = "Benchmark 02";
static const String BENCHMARK_03_STR = "Benchmark 03";
static const String BENCHMARK_04_STR = "Benchmark 04";

void AppState_MainScreen::HandleButtonPressed(StringHash eventType, VariantMap& eventData)
{
    Button* pressedButton = static_cast<Button*>(eventData["Element"].GetPtr());

    if (pressedButton->GetName() == BENCHMARK_01_STR)
        APP_STATE_MANAGER->SetRequiredAppStateId(APPSTATEID_BENCHMARK01);
    else if (pressedButton->GetName() == BENCHMARK_02_STR)
        APP_STATE_MANAGER->SetRequiredAppStateId(APPSTATEID_BENCHMARK02);
    else if (pressedButton->GetName() == BENCHMARK_03_STR)
        APP_STATE_MANAGER->SetRequiredAppStateId(APPSTATEID_BENCHMARK03);
    else if (pressedButton->GetName() == BENCHMARK_04_STR)
        APP_STATE_MANAGER->SetRequiredAppStateId(APPSTATEID_BENCHMARK04);
}

void AppState_MainScreen::CreateButton(const String& name, const String& text, Window& parent)
{
    Button* button = parent.create_child<Button>(name);
    button->SetStyleAuto();
    button->SetFixedHeight(24);

    Text* buttonText = button->create_child<Text>();
    buttonText->SetStyleAuto();
    buttonText->SetText(text);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);

    subscribe_to_event(button, E_RELEASED, DV_HANDLER(AppState_MainScreen, HandleButtonPressed));
}

void AppState_MainScreen::CreateGui()
{
    UiElement* root = DV_UI->GetRoot();

    Window* window = root->create_child<Window>(MAIN_SCREEN_WINDOW_STR);
    window->SetStyleAuto();
    window->SetMinWidth(384);
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetPosition(10, 34);

    Text* windowTitle = window->create_child<Text>();
    windowTitle->SetStyleAuto();
    windowTitle->SetText("Benchmark list");

    CreateButton(BENCHMARK_01_STR, APP_STATE_MANAGER->GetName(APPSTATEID_BENCHMARK01), *window);
    CreateButton(BENCHMARK_02_STR, APP_STATE_MANAGER->GetName(APPSTATEID_BENCHMARK02), *window);
    CreateButton(BENCHMARK_03_STR, APP_STATE_MANAGER->GetName(APPSTATEID_BENCHMARK03), *window);
    CreateButton(BENCHMARK_04_STR, APP_STATE_MANAGER->GetName(APPSTATEID_BENCHMARK04), *window);
}

void AppState_MainScreen::DestroyGui()
{
    UiElement* root = DV_UI->GetRoot();
    Window* window = root->GetChildStaticCast<Window>(MAIN_SCREEN_WINDOW_STR);
    window->Remove();
}

void AppState_MainScreen::OnEnter()
{
    assert(!scene_);
    LoadSceneXml("benchmark/scenes/main_screen.xml");

    CreateGui();
    setup_viewport();
    DV_INPUT->SetMouseVisible(true);
    scene_update.connect(scene_->scene_update, bind(&AppState_MainScreen::handle_scene_update, this, placeholders::_1, placeholders::_2));
    fpsCounter_.Clear();
}

void AppState_MainScreen::OnLeave()
{
    DestroyViewport();
    DestroyGui();
    scene_ = nullptr;
}

void AppState_MainScreen::handle_scene_update(Scene* scene, float time_step)
{
    fpsCounter_.Update(time_step);
    UpdateCurrentFpsElement();

    if (DV_INPUT->GetKeyPress(KEY_ESCAPE))
        DV_ENGINE->Exit();
}
