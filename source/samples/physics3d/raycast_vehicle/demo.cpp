// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/terrain.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/io/file_system.h>
#include <dviglo/input/input.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/constraint.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/raycast_vehicle.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "demo.h"
#include "vehicle.h"

#include <dviglo/common/debug_new.h>

const float CAMERA_DISTANCE = 10.0f;

DV_DEFINE_APPLICATION_MAIN(RaycastVehicleDemo)

RaycastVehicleDemo::RaycastVehicleDemo()
{
    // Register factory and attributes for the Vehicle component so it can be created via create_component, and loaded / saved
    Vehicle::register_object();
}

void RaycastVehicleDemo::Start()
{
    // Execute base class startup
    Sample::Start();
    // Create static scene content
    create_scene();
    // Create the controllable vehicle
    CreateVehicle();
    // Create the UI content
    create_instructions();
    // Subscribe to necessary events
    subscribe_to_events();
    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void RaycastVehicleDemo::create_scene()
{
    ResourceCache* cache = DV_RES_CACHE;
    scene_ = new Scene();
    // Create scene subsystem components
    scene_->create_component<Octree>();
    scene_->create_component<PhysicsWorld>();
    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node();
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(500.0f);
    DV_RENDERER->SetViewport(0, new Viewport(scene_, camera));
    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);
    // Create heightmap terrain with collision
    Node* terrainNode = scene_->create_child("Terrain");
    terrainNode->SetPosition(Vector3::ZERO);
    auto* terrain = terrainNode->create_component<Terrain>();
    terrain->SetPatchSize(64);
    terrain->SetSpacing(Vector3(3.0f, 0.1f, 3.0f)); // Spacing between vertices and vertical resolution of the height map
    terrain->SetSmoothing(true);
    terrain->SetHeightMap(cache->GetResource<Image>("textures/heightmap.png"));
    terrain->SetMaterial(cache->GetResource<Material>("materials/terrain.xml"));
    // The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
    // terrain patches and other objects behind it
    terrain->SetOccluder(true);
    auto* body = terrainNode->create_component<RigidBody>();
    body->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry
    auto* shape =
        terrainNode->create_component<CollisionShape>();
    shape->SetTerrain();
    // Create 1000 mushrooms in the terrain. Always face outward along the terrain normal
    const unsigned NUM_MUSHROOMS = 1000;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* objectNode = scene_->create_child("Mushroom");
        Vector3 position(Random(2000.0f) - 1000.0f, 0.0f, Random(2000.0f) - 1000.0f);
        position.y = terrain->GetHeight(position) - 0.1f;
        objectNode->SetPosition(position);
        // Create a rotation quaternion from up vector to terrain normal
        objectNode->SetRotation(Quaternion(Vector3::UP, terrain->GetNormal(position)));
        objectNode->SetScale(3.0f);
        auto* object = objectNode->create_component<StaticModel>();
        object->SetModel(cache->GetResource<Model>("models/mushroom.mdl"));
        object->SetMaterial(cache->GetResource<Material>("materials/mushroom.xml"));
        object->SetCastShadows(true);
        auto* body = objectNode->create_component<RigidBody>();
        body->SetCollisionLayer(2);
        auto* shape = objectNode->create_component<CollisionShape>();
        shape->SetTriangleMesh(object->GetModel(), 0);
    }
}

void RaycastVehicleDemo::CreateVehicle()
{
    Node* vehicleNode = scene_->create_child("Vehicle");
    vehicleNode->SetPosition(Vector3(0.0f, 25.0f, 0.0f));
    // Create the vehicle logic component
    vehicle_ = vehicleNode->create_component<Vehicle>();
    // Create the rendering and physics components
    vehicle_->Init();
}

void RaycastVehicleDemo::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys to drive, F to brake, mouse to rotate camera\n"
        "F5 to save scene, F7 to load");
    instructionText->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);
    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void RaycastVehicleDemo::subscribe_to_events()
{
    // Subscribe to Update event for setting the vehicle controls before physics simulation
    subscribe_to_event(E_UPDATE, DV_HANDLER(RaycastVehicleDemo, handle_update));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(RaycastVehicleDemo, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void RaycastVehicleDemo::handle_update(StringHash eventType,
                                 VariantMap& eventData)
{
    using namespace Update;
    Input* input = DV_INPUT;
    if (vehicle_)
    {
        // Get movement controls and assign them to the vehicle component. If UI has a focused element, clear controls
        if (!DV_UI->GetFocusElement())
        {
            vehicle_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
            vehicle_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
            vehicle_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
            vehicle_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            vehicle_->controls_.Set(CTRL_BRAKE, input->GetKeyDown(KEY_F));
            // Add yaw & pitch from the mouse motion. Used only for the camera, does not affect motion
            vehicle_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
            vehicle_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
            // Limit pitch
            vehicle_->controls_.pitch_ = Clamp(vehicle_->controls_.pitch_, 0.0f, 80.0f);
            // Check for loading / saving the scene
            if (input->GetKeyPress(KEY_F5))
            {
                File saveFile(DV_FILE_SYSTEM->GetProgramDir() + "data/scenes/raycast_vehicle_demo.xml",
                              FILE_WRITE);
                scene_->save_xml(saveFile);
            }
            if (input->GetKeyPress(KEY_F7))
            {
                File loadFile(DV_FILE_SYSTEM->GetProgramDir() + "data/scenes/raycast_vehicle_demo.xml",
                              FILE_READ);
                scene_->load_xml(loadFile);
                // After loading we have to reacquire the weak pointer to the Vehicle component, as it has been recreated
                // Simply find the vehicle's scene node by name as there's only one of them
                Node* vehicleNode = scene_->GetChild("Vehicle", true);
                if (vehicleNode)
                {
                    vehicle_ = vehicleNode->GetComponent<Vehicle>();
                }
            }
        }
        else
        {
            vehicle_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_BRAKE, false);
        }
    }
}

void RaycastVehicleDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!vehicle_)
    {
        return;
    }
    Node* vehicleNode = vehicle_->GetNode();
    // Physics update has completed. Position camera behind vehicle
    Quaternion dir(vehicleNode->GetRotation().YawAngle(), Vector3::UP);
    dir = dir * Quaternion(vehicle_->controls_.yaw_, Vector3::UP);
    dir = dir * Quaternion(vehicle_->controls_.pitch_, Vector3::RIGHT);
    Vector3 cameraTargetPos =
        vehicleNode->GetPosition() - dir * Vector3(0.0f, 0.0f, CAMERA_DISTANCE);
    Vector3 cameraStartPos = vehicleNode->GetPosition();
    // Raycast camera against static objects (physics collision mask 2)
    // and move it closer to the vehicle if something in between
    Ray cameraRay(cameraStartPos, cameraTargetPos - cameraStartPos);
    float cameraRayLength = (cameraTargetPos - cameraStartPos).Length();
    PhysicsRaycastResult result;
    scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, cameraRay, cameraRayLength, 2);
    if (result.body_)
    {
        cameraTargetPos = cameraStartPos + cameraRay.direction_ * (result.distance_ - 0.5f);
    }
    cameraNode_->SetPosition(cameraTargetPos);
    cameraNode_->SetRotation(dir);
}
