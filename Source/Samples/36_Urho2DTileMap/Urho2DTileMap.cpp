// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/engine/engine.h>
#include <dviglo/ui/font.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/input/input.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/urho_2d/static_sprite_2d.h>
#include <dviglo/ui/text.h>
#include <dviglo/urho_2d/tilemap_2d.h>
#include <dviglo/urho_2d/tilemap_layer_2d.h>
#include <dviglo/urho_2d/tmx_file_2d.h>

#include "Urho2DTileMap.h"

#include <dviglo/debug_new.h>

URHO3D_DEFINE_APPLICATION_MAIN(Urho2DTileMap)

Urho2DTileMap::Urho2DTileMap(Context* context) :
    Sample(context)
{
}

void Urho2DTileMap::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Urho2DTileMap::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    auto* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.0f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.0) is set for full visibility at 1280x800 resolution)

    auto* cache = GetSubsystem<ResourceCache>();
    // Get tmx file
    auto* tmxFile = cache->GetResource<TmxFile2D>("Urho2D/isometric_grass_and_water.tmx");
    if (!tmxFile)
        return;

    SharedPtr<Node> tileMapNode(scene_->CreateChild("TileMap"));
    tileMapNode->SetPosition(Vector3(0.0f, 0.0f, -1.0f));

    auto* tileMap = tileMapNode->CreateComponent<TileMap2D>();
    // Set animation
    tileMap->SetTmxFile(tmxFile);

    // Set camera's position
    const TileMapInfo2D& info = tileMap->GetInfo();
    float x = info.GetMapWidth() * 0.5f;
    float y = info.GetMapHeight() * 0.5f;
    cameraNode_->SetPosition(Vector3(x, y, -10.0f));
}

void Urho2DTileMap::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.\n LMB to remove a tile, RMB to swap grass and water.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Urho2DTileMap::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Urho2DTileMap::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Urho2DTileMap::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Urho2DTileMap, HandleUpdate));

    // Listen to mouse clicks
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Urho2DTileMap, HandleMouseButtonDown));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void Urho2DTileMap::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void Urho2DTileMap::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    auto* input = GetSubsystem<Input>();

    Node* tileMapNode = scene_->GetChild("TileMap", true);
    auto* map = tileMapNode->GetComponent<TileMap2D>();
    TileMapLayer2D* layer = map->GetLayer(0);

    Vector2 pos = GetMousePositionXY();
    int x, y;
    if (map->PositionToTileIndex(x, y, pos))
    {
        // Get tile's sprite. Note that layer.GetTile(x, y).sprite is read-only, so we get the sprite through tile's node
        Node* n = layer->GetTileNode(x, y);
        if (!n)
            return;
        auto* sprite = n->GetComponent<StaticSprite2D>();

        if (input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            // Swap grass and water
            if (layer->GetTile(x, y)->GetGid() < 9) // First 8 sprites in the "isometric_grass_and_water.png" tileset are mostly grass and from 9 to 24 they are mostly water
                sprite->SetSprite(layer->GetTile(0, 0)->GetSprite()); // Replace grass by water sprite used in top tile
            else
                sprite->SetSprite(layer->GetTile(24, 24)->GetSprite()); // Replace water by grass sprite used in bottom tile
        }
        else
        {
            sprite->SetSprite(nullptr); // 'Remove' sprite
        }
    }
}

Vector2 Urho2DTileMap::GetMousePositionXY()
{
    auto* input = GetSubsystem<Input>();
    auto* graphics = GetSubsystem<Graphics>();
    auto* camera = cameraNode_->GetComponent<Camera>();
    Vector3 screenPoint = Vector3((float)input->GetMousePosition().x_ / graphics->GetWidth(), (float)input->GetMousePosition().y_ / graphics->GetHeight(), 10.0f);
    Vector3 worldPoint = camera->ScreenToWorldPoint(screenPoint);
    return Vector2(worldPoint.x_, worldPoint.y_);
}
