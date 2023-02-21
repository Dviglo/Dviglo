// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/technique.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/check_box.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/list_view.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/tooltip.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_component.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>

#include "3d.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Hello3DUI)

Hello3DUI::Hello3DUI() :
    uiRoot_(GetSubsystem<UI>()->GetRoot()),
    dragBeginPosition_(IntVector2::ZERO),
    animateCube_(true),
    renderOnCube_(false),
    drawDebug_(false)
{
}

void Hello3DUI::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    auto* cache = GetSubsystem<ResourceCache>();
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);

    // Initialize Scene
    InitScene();

    // Initialize Window
    InitWindow();

    // Create and add some controls to the Window
    InitControls();

    // Create a draggable Fish
    CreateDraggableFish();

    // Create 3D UI rendered on a cube.
    Init3DUI();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Hello3DUI::InitControls()
{
    // Create a CheckBox
    auto* checkBox = new CheckBox();
    checkBox->SetName("CheckBox");

    // Create a Button
    auto* button = new Button();
    button->SetName("Button");
    button->SetMinHeight(24);

    // Create a LineEdit
    auto* lineEdit = new LineEdit();
    lineEdit->SetName("LineEdit");
    lineEdit->SetMinHeight(24);

    // Add controls to Window
    window_->AddChild(checkBox);
    window_->AddChild(button);
    window_->AddChild(lineEdit);

    // Apply previously set default style
    checkBox->SetStyleAuto();
    button->SetStyleAuto();
    lineEdit->SetStyleAuto();

    instructions_ = new Text();
    instructions_->SetStyleAuto();
    instructions_->SetText("[TAB]   - toggle between rendering on screen or cube.\n"
                           "[Space] - toggle cube rotation.");
    uiRoot_->AddChild(instructions_);
}

void Hello3DUI::InitWindow()
{
    // Create the Window and add it to the UI's root node
    window_ = new Window();
    uiRoot_->AddChild(window_);

    // Set Window size and layout settings
    window_->SetMinWidth(384);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetAlignment(HA_CENTER, VA_CENTER);
    window_->SetName("Window");

    // Create Window 'titlebar' container
    auto* titleBar = new UIElement();
    titleBar->SetMinSize(0, 24);
    titleBar->SetVerticalAlignment(VA_TOP);
    titleBar->SetLayoutMode(LM_HORIZONTAL);

    // Create the Window title Text
    auto* windowTitle = new Text();
    windowTitle->SetName("WindowTitle");
    windowTitle->SetText("Hello GUI!");

    // Create the Window's close button
    auto* buttonClose = new Button();
    buttonClose->SetName("CloseButton");

    // Add the controls to the title bar
    titleBar->AddChild(windowTitle);
    titleBar->AddChild(buttonClose);

    // Add the title bar to the Window
    window_->AddChild(titleBar);

    // Create a list.
    auto* list = window_->CreateChild<ListView>();
    list->SetSelectOnClickEnd(true);
    list->SetHighlightMode(HM_ALWAYS);
    list->SetMinHeight(200);

    for (int i = 0; i < 32; i++)
    {
        auto* text = new Text();
        text->SetStyleAuto();
        text->SetText(ToString("List item %d", i));
        text->SetName(ToString("Item %d", i));
        list->AddItem(text);
    }

    // Apply styles
    window_->SetStyleAuto();
    list->SetStyleAuto();
    windowTitle->SetStyleAuto();
    buttonClose->SetStyle("CloseButton");

    // Subscribe to buttonClose release (following a 'press') events
    SubscribeToEvent(buttonClose, E_RELEASED, DV_HANDLER(Hello3DUI, HandleClosePressed));

    // Subscribe also to all UI mouse clicks just to see where we have clicked
    SubscribeToEvent(E_UIMOUSECLICK, DV_HANDLER(Hello3DUI, HandleControlClicked));
}

void Hello3DUI::InitScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene();
    scene_->CreateComponent<Octree>();
    auto* zone = scene_->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetFogColor(Color::GRAY);
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a child scene node (at world origin) and a StaticModel component into it.
    Node* boxNode = scene_->CreateChild("Box");
    boxNode->SetScale(Vector3(5.0f, 5.0f, 5.0f));
    boxNode->SetRotation(Quaternion(90, Vector3::LEFT));

    // Create a box model and hide it initially.
    auto* boxModel = boxNode->CreateComponent<StaticModel>();
    boxModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxNode->SetEnabled(false);

    // Create a camera.
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node.
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    // Set up a viewport so 3D scene can be visible.
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    // Subscribe to update event and animate cube and handle input.
    SubscribeToEvent(E_UPDATE, DV_HANDLER(Hello3DUI, HandleUpdate));
}

void Hello3DUI::CreateDraggableFish()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* graphics = GetSubsystem<Graphics>();

    // Create a draggable Fish button
    auto* draggableFish = new Button();
    draggableFish->SetTexture(cache->GetResource<Texture2D>("Textures/UrhoDecal.dds")); // Set texture
    draggableFish->SetBlendMode(BLEND_ADD);
    draggableFish->SetSize(128, 128);
    draggableFish->SetPosition((graphics->GetWidth() - draggableFish->GetWidth()) / 2, 200);
    draggableFish->SetName("Fish");
    uiRoot_->AddChild(draggableFish);

    // Add a tooltip to Fish button
    auto* toolTip = new ToolTip();
    draggableFish->AddChild(toolTip);
    toolTip->SetPosition(IntVector2(draggableFish->GetWidth() + 5, draggableFish->GetWidth() / 2)); // slightly offset from close button
    auto* textHolder = new BorderImage();
    toolTip->AddChild(textHolder);
    textHolder->SetStyle("ToolTipBorderImage");
    auto* toolTipText = new Text();
    textHolder->AddChild(toolTipText);
    toolTipText->SetStyle("ToolTipText");
    toolTipText->SetText("Please drag me!");

    // Subscribe draggableFish to Drag Events (in order to make it draggable)
    // See "Event list" in documentation's Main Page for reference on available Events and their eventData
    SubscribeToEvent(draggableFish, E_DRAGBEGIN, DV_HANDLER(Hello3DUI, HandleDragBegin));
    SubscribeToEvent(draggableFish, E_DRAGMOVE, DV_HANDLER(Hello3DUI, HandleDragMove));
    SubscribeToEvent(draggableFish, E_DRAGEND, DV_HANDLER(Hello3DUI, HandleDragEnd));
}

void Hello3DUI::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    // Get UIElement relative position where input (touch or click) occurred (top-left = IntVector2(0,0))
    dragBeginPosition_ = IntVector2(eventData["ElementX"].GetI32(), eventData["ElementY"].GetI32());
}

void Hello3DUI::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetI32(), eventData["Y"].GetI32());
    UIElement* draggedElement = static_cast<UIElement*>(eventData["Element"].GetPtr());
    draggedElement->SetPosition(dragCurrentPosition - dragBeginPosition_);
}

void Hello3DUI::HandleDragEnd(StringHash eventType, VariantMap& eventData) // For reference (not used here)
{
}

void Hello3DUI::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    if (GetPlatform() != "Web")
        engine_->Exit();
}

void Hello3DUI::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    // Get the Text control acting as the Window's title
    auto* windowTitle = window_->GetChildStaticCast<Text>("WindowTitle", true);

    // Get control that was clicked
    auto* clicked = static_cast<UIElement*>(eventData[UIMouseClick::P_ELEMENT].GetPtr());

    String name = "...?";
    if (clicked)
    {
        // Get the name of the control that was clicked
        name = clicked->GetName();
    }

    // Update the Window's title text
    windowTitle->SetText("Hello " + name + "!");
}

void Hello3DUI::Init3DUI()
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Node that will get UI rendered on it.
    Node* boxNode = scene_->GetChild("Box");
    // Create a component that sets up UI rendering. It sets material to StaticModel of the node.
    auto* component = boxNode->CreateComponent<UIComponent>();
    // Optionally modify material. Technique is changed so object is visible without any lights.
    component->GetMaterial()->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffUnlit.xml"));
    // Save root element of texture UI for later use.
    textureRoot_ = component->GetRoot();
    // Set size of root element. This is size of texture as well.
    textureRoot_->SetSize(512, 512);
}

void Hello3DUI::HandleUpdate(StringHash, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    auto* input = GetSubsystem<Input>();
    Node* node = scene_->GetChild("Box");

    if (current_.NotNull() && drawDebug_)
        GetSubsystem<UI>()->DebugDraw(current_);

    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        current_ = GetSubsystem<UI>()->GetElementAt(input->GetMousePosition());

    if (input->GetKeyPress(KEY_TAB))
    {
        renderOnCube_ = !renderOnCube_;
        // Toggle between rendering on screen or to texture.
        if (renderOnCube_)
        {
            node->SetEnabled(true);
            textureRoot_->AddChild(window_);
        }
        else
        {
            node->SetEnabled(false);
            uiRoot_->AddChild(window_);
        }
    }

    if (input->GetKeyPress(KEY_SPACE))
        animateCube_ = !animateCube_;

    if (input->GetKeyPress(KEY_F2))
        drawDebug_ = !drawDebug_;

    if (animateCube_)
    {
        node->Yaw(6.0f * timeStep * 1.5f);
        node->Roll(-6.0f * timeStep * 1.5f);
        node->Pitch(-6.0f * timeStep * 1.5f);
    }
}
