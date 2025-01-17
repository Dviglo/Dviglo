// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation_controller.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/navigation/crowd_agent.h>
#include <dviglo/navigation/dynamic_navigation_mesh.h>
#include <dviglo/navigation/navigable.h>
#include <dviglo/navigation/navigation_events.h>
#include <dviglo/navigation/obstacle.h>
#include <dviglo/navigation/off_mesh_connection.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "crowd_navigation.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(CrowdNavigation)

CrowdNavigation::CrowdNavigation()
{
}

void CrowdNavigation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_ui();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update and render post-update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void CrowdNavigation::create_scene()
{
    ResourceCache* cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("models/plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("materials/stone_tiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create randomly sized boxes. If boxes are big enough, make them occluders
    Node* boxGroup = scene_->create_child("Boxes");
    for (unsigned i = 0; i < 20; ++i)
    {
        Node* boxNode = boxGroup->create_child("Box");
        float size = 1.0f + Random(10.0f);
        boxNode->SetPosition(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f));
        boxNode->SetScale(size);
        auto* boxObject = boxNode->create_component<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("models/box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("materials/stone.xml"));
        boxObject->SetCastShadows(true);
        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create a DynamicNavigationMesh component to the scene root
    auto* navMesh = scene_->create_component<DynamicNavigationMesh>();
    // Set small tiles to show navigation mesh streaming
    navMesh->SetTileSize(32);
    // Enable drawing debug geometry for obstacles and off-mesh connections
    navMesh->SetDrawObstacles(true);
    navMesh->SetDrawOffMeshConnections(true);
    // Set the agent height large enough to exclude the layers under boxes
    navMesh->SetAgentHeight(10.0f);
    // Set nav mesh cell height to minimum (allows agents to be grounded)
    navMesh->SetCellHeight(0.05f);
    // Create a Navigable component to the scene root. This tags all of the geometry in the scene as being part of the
    // navigation mesh. By default this is recursive, but the recursion could be turned off from Navigable
    scene_->create_component<Navigable>();
    // Add padding to the navigation mesh in Y-direction so that we can add objects on top of the tallest boxes
    // in the scene and still update the mesh correctly
    navMesh->SetPadding(Vector3(0.0f, 10.0f, 0.0f));
    // Now build the navigation geometry. This will take some time. Note that the navigation mesh will prefer to use
    // physics geometry from the scene nodes, as it often is simpler, but if it can not find any (like in this example)
    // it will use renderable geometry instead
    navMesh->Build();

    // Create an off-mesh connection to each box to make them climbable (tiny boxes are skipped). A connection is built from 2 nodes.
    // Note that OffMeshConnections must be added before building the navMesh, but as we are adding Obstacles next, tiles will be automatically rebuilt.
    // Creating connections post-build here allows us to use FindNearestPoint() to procedurally set accurate positions for the connection
    CreateBoxOffMeshConnections(navMesh, boxGroup);

    // Create some mushrooms as obstacles. Note that obstacles are non-walkable areas
    for (unsigned i = 0; i < 100; ++i)
        create_mushroom(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));

    // Create a CrowdManager component to the scene root
    auto* crowdManager = scene_->create_component<CrowdManager>();
    CrowdObstacleAvoidanceParams params = crowdManager->GetObstacleAvoidanceParams(0);
    // Set the params to "High (66)" setting
    params.velBias = 0.5f;
    params.adaptiveDivs = 7;
    params.adaptiveRings = 3;
    params.adaptiveDepth = 3;
    crowdManager->SetObstacleAvoidanceParams(0, params);

    // Create some movable barrels. We create them as crowd agents, as for moving entities it is less expensive and more convenient than using obstacles
    CreateMovingBarrels(navMesh);

    // Create Jack node as crowd agent
    SpawnJack(Vector3(-5.0f, 0.0f, 20.0f), scene_->create_child("Jacks"));

    // Create the camera. Set far clip to match the fog. Note: now we actually create the camera node outside the scene, because
    // we want it to be unaffected by scene load / save
    cameraNode_ = new Node();
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane and looking down
    cameraNode_->SetPosition(Vector3(0.0f, 50.0f, 0.0f));
    pitch_ = 80.0f;
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
}

void CrowdNavigation::create_ui()
{
    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will point the raycast target
    auto* style = DV_RES_CACHE->GetResource<XmlFile>("ui/default_style.xml");
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto(style);
    DV_UI->SetCursor(cursor);

    // Set starting position of the cursor at the rendering window center
    Graphics* graphics = DV_GRAPHICS;
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Construct new Text object, set string to display and font to use
    instructionText_ = DV_UI->GetRoot()->create_child<Text>();
    instructionText_->SetText(
        "Use WASD keys to move, RMB to rotate view\n"
        "LMB to set destination, SHIFT+LMB to spawn a Jack\n"
        "MMB or O key to add obstacles or remove obstacles/agents\n"
        "F5 to save scene, F7 to load\n"
        "Tab to toggle navigation mesh streaming\n"
        "Space to toggle debug geometry\n"
        "F12 to toggle this instruction text"
    );
    instructionText_->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText_->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText_->SetHorizontalAlignment(HA_CENTER);
    instructionText_->SetVerticalAlignment(VA_CENTER);
    instructionText_->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void CrowdNavigation::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);
}

void CrowdNavigation::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(CrowdNavigation, handle_update));

    // Subscribe handle_post_render_update() function for processing the post-render update event, during which we request debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(CrowdNavigation, handle_post_render_update));

    // Subscribe HandleCrowdAgentFailure() function for resolving invalidation issues with agents, during which we
    // use a larger extents for finding a point on the navmesh to fix the agent's position
    subscribe_to_event(E_CROWD_AGENT_FAILURE, DV_HANDLER(CrowdNavigation, HandleCrowdAgentFailure));

    // Subscribe HandleCrowdAgentReposition() function for controlling the animation
    subscribe_to_event(E_CROWD_AGENT_REPOSITION, DV_HANDLER(CrowdNavigation, HandleCrowdAgentReposition));

    // Subscribe HandleCrowdAgentFormation() function for positioning agent into a formation
    subscribe_to_event(E_CROWD_AGENT_FORMATION, DV_HANDLER(CrowdNavigation, HandleCrowdAgentFormation));
}

void CrowdNavigation::SpawnJack(const Vector3& pos, Node* jackGroup)
{
    SharedPtr<Node> jackNode(jackGroup->create_child("Jack"));
    jackNode->SetPosition(pos);
    auto* modelObject = jackNode->create_component<AnimatedModel>();
    modelObject->SetModel(DV_RES_CACHE->GetResource<Model>("models/jack.mdl"));
    modelObject->SetMaterial(DV_RES_CACHE->GetResource<Material>("materials/jack.xml"));
    modelObject->SetCastShadows(true);
    jackNode->create_component<AnimationController>();

    // Create a CrowdAgent component and set its height and realistic max speed/acceleration. Use default radius
    auto* agent = jackNode->create_component<CrowdAgent>();
    agent->SetHeight(2.0f);
    agent->SetMaxSpeed(3.0f);
    agent->SetMaxAccel(5.0f);
}

void CrowdNavigation::create_mushroom(const Vector3& pos)
{
    Node* mushroomNode = scene_->create_child("Mushroom");
    mushroomNode->SetPosition(pos);
    mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
    mushroomNode->SetScale(2.0f + Random(0.5f));
    auto* mushroomObject = mushroomNode->create_component<StaticModel>();
    mushroomObject->SetModel(DV_RES_CACHE->GetResource<Model>("models/mushroom.mdl"));
    mushroomObject->SetMaterial(DV_RES_CACHE->GetResource<Material>("materials/mushroom.xml"));
    mushroomObject->SetCastShadows(true);

    // Create the navigation Obstacle component and set its height & radius proportional to scale
    auto* obstacle = mushroomNode->create_component<Obstacle>();
    obstacle->SetRadius(mushroomNode->GetScale().x);
    obstacle->SetHeight(mushroomNode->GetScale().y);
}

void CrowdNavigation::CreateBoxOffMeshConnections(DynamicNavigationMesh* navMesh, Node* boxGroup)
{
    const Vector<SharedPtr<Node>>& boxes = boxGroup->GetChildren();
    for (const SharedPtr<Node>& box : boxes)
    {
        Vector3 boxPos = box->GetPosition();
        float boxHalfSize = box->GetScale().x / 2;

        // Create 2 empty nodes for the start & end points of the connection. Note that order matters only when using one-way/unidirectional connection.
        Node* connectionStart = box->create_child("ConnectionStart");
        connectionStart->SetWorldPosition(navMesh->FindNearestPoint(boxPos + Vector3(boxHalfSize, -boxHalfSize, 0))); // Base of box
        Node* connectionEnd = connectionStart->create_child("ConnectionEnd");
        connectionEnd->SetWorldPosition(navMesh->FindNearestPoint(boxPos + Vector3(boxHalfSize, boxHalfSize, 0))); // Top of box

        // Create the OffMeshConnection component to one node and link the other node
        auto* connection = connectionStart->create_component<OffMeshConnection>();
        connection->SetEndPoint(connectionEnd);
    }
}

void CrowdNavigation::CreateMovingBarrels(DynamicNavigationMesh* navMesh)
{
    Node* barrel = scene_->create_child("Barrel");
    auto* model = barrel->create_component<StaticModel>();
    model->SetModel(DV_RES_CACHE->GetResource<Model>("models/cylinder.mdl"));
    auto* material = DV_RES_CACHE->GetResource<Material>("materials/stone_tiled.xml");
    model->SetMaterial(material);
    material->SetTexture(TU_DIFFUSE, DV_RES_CACHE->GetResource<Texture2D>("textures/terrain_detail2.dds"));
    model->SetCastShadows(true);
    for (unsigned i = 0;  i < 20; ++i)
    {
        Node* clone = barrel->Clone();
        float size = 0.5f + Random(1.0f);
        clone->SetScale(Vector3(size / 1.5f, size * 2.0f, size / 1.5f));
        clone->SetPosition(navMesh->FindNearestPoint(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f)));
        auto* agent = clone->create_component<CrowdAgent>();
        agent->SetRadius(clone->GetScale().x * 0.5f);
        agent->SetHeight(size);
        agent->SetNavigationQuality(NAVIGATIONQUALITY_LOW);
    }
    barrel->Remove();
}

void CrowdNavigation::SetPathPoint(bool spawning)
{
    Vector3 hitPos;
    Drawable* hitDrawable;

    if (Raycast(250.0f, hitPos, hitDrawable))
    {
        auto* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
        Vector3 pathPos = navMesh->FindNearestPoint(hitPos, Vector3(1.0f, 1.0f, 1.0f));
        Node* jackGroup = scene_->GetChild("Jacks");
        if (spawning)
            // Spawn a jack at the target position
            SpawnJack(pathPos, jackGroup);
        else
            // Set crowd agents target position
            scene_->GetComponent<CrowdManager>()->SetCrowdTarget(pathPos, jackGroup);
    }
}

void CrowdNavigation::AddOrRemoveObject()
{
    // Raycast and check if we hit a mushroom node. If yes, remove it, if no, create a new one
    Vector3 hitPos;
    Drawable* hitDrawable;

    if (Raycast(250.0f, hitPos, hitDrawable))
    {
        Node* hitNode = hitDrawable->GetNode();

        // Note that navmesh rebuild happens when the Obstacle component is removed
        if (hitNode->GetName() == "Mushroom")
            hitNode->Remove();
        else if (hitNode->GetName() == "Jack")
            hitNode->Remove();
        else
            create_mushroom(hitPos);
    }
}

bool CrowdNavigation::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable)
{
    hitDrawable = nullptr;

    UI* ui = DV_UI;
    IntVector2 pos = ui->GetCursorPosition();
    // Check the cursor is visible and there is no UI element in front of the cursor
    if (!ui->GetCursor()->IsVisible() || ui->GetElementAt(pos, true))
        return false;

    pos = ui->ConvertUIToSystem(pos);

    Graphics* graphics = DV_GRAPHICS;
    auto* camera = cameraNode_->GetComponent<Camera>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x / graphics->GetWidth(), (float)pos.y / graphics->GetHeight());
    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    Vector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DrawableTypes::Geometry);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.Size())
    {
        RayQueryResult& result = results[0];
        hitPos = result.position_;
        hitDrawable = result.drawable_;
        return true;
    }

    return false;
}

void CrowdNavigation::move_camera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    UI* ui = DV_UI;
    Input* input = DV_INPUT;
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element (the console)
    if (ui->GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed.
    // Используем скан-коды, а не коды клавиш, иначе не будет работать в Linux, когда включена русская раскладка клавиатуры
    if (input->GetScancodeDown(SCANCODE_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Set destination or spawn a new jack with left mouse button
    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        SetPathPoint(input->GetQualifierDown(QUAL_SHIFT));
    // Add new obstacle or remove existing obstacle/agent with middle mouse button
    else if (input->GetMouseButtonPress(MOUSEB_MIDDLE) || input->GetScancodePress(SCANCODE_O))
        AddOrRemoveObject();

    // Check for loading/saving the scene from/to the file data/scenes/crowd_navigation.xml relative to the executable directory
    if (input->GetKeyPress(KEY_F5))
    {
        File saveFile(DV_FILE_SYSTEM->GetProgramDir() + "data/scenes/crowd_navigation.xml", FILE_WRITE);
        scene_->save_xml(saveFile);
    }
    else if (input->GetKeyPress(KEY_F7))
    {
        File loadFile(DV_FILE_SYSTEM->GetProgramDir() + "data/scenes/crowd_navigation.xml", FILE_READ);
        scene_->load_xml(loadFile);
    }

    // Toggle debug geometry with space
    else if (input->GetKeyPress(KEY_SPACE))
        draw_debug_ = !draw_debug_;

    // Toggle instruction text with F12
    else if (input->GetKeyPress(KEY_F12))
    {
        if (instructionText_)
            instructionText_->SetVisible(!instructionText_->IsVisible());
    }
}

void CrowdNavigation::toggle_streaming(bool enabled)
{
    auto* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
    if (enabled)
    {
        int maxTiles = (2 * streamingDistance_ + 1) * (2 * streamingDistance_ + 1);
        BoundingBox boundingBox = navMesh->GetBoundingBox();
        save_navigation_data();
        navMesh->Allocate(boundingBox, maxTiles);
    }
    else
        navMesh->Build();
}

void CrowdNavigation::update_streaming()
{
    // Center the navigation mesh at the crowd of jacks
    Vector3 averageJackPosition;
    if (Node* jackGroup = scene_->GetChild("Jacks"))
    {
        const unsigned numJacks = jackGroup->GetNumChildren();
        for (unsigned i = 0; i < numJacks; ++i)
            averageJackPosition += jackGroup->GetChild(i)->GetWorldPosition();
        averageJackPosition /= (float)numJacks;
    }

    // Compute currently loaded area
    auto* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
    const IntVector2 jackTile = navMesh->GetTileIndex(averageJackPosition);
    const IntVector2 numTiles = navMesh->GetNumTiles();
    const IntVector2 beginTile = VectorMax(IntVector2::ZERO, jackTile - IntVector2::ONE * streamingDistance_);
    const IntVector2 endTile = VectorMin(jackTile + IntVector2::ONE * streamingDistance_, numTiles - IntVector2::ONE);

    // Remove tiles
    for (HashSet<IntVector2>::Iterator i = addedTiles_.Begin(); i != addedTiles_.End();)
    {
        const IntVector2 tileIdx = *i;
        if (beginTile.x <= tileIdx.x && tileIdx.x <= endTile.x && beginTile.y <= tileIdx.y && tileIdx.y <= endTile.y)
            ++i;
        else
        {
            navMesh->RemoveTile(tileIdx);
            i = addedTiles_.Erase(i);
        }
    }

    // Add tiles
    for (int z = beginTile.y; z <= endTile.y; ++z)
        for (int x = beginTile.x; x <= endTile.x; ++x)
        {
            const IntVector2 tileIdx(x, z);
            if (!navMesh->HasTile(tileIdx) && tileData_.Contains(tileIdx))
            {
                addedTiles_.Insert(tileIdx);
                navMesh->AddTile(tileData_[tileIdx]);
            }
        }
}

void CrowdNavigation::save_navigation_data()
{
    auto* navMesh = scene_->GetComponent<DynamicNavigationMesh>();
    tileData_.Clear();
    addedTiles_.Clear();
    const IntVector2 numTiles = navMesh->GetNumTiles();
    for (int z = 0; z < numTiles.y; ++z)
        for (int x = 0; x <= numTiles.x; ++x)
        {
            const IntVector2 tileIdx = IntVector2(x, z);
            tileData_[tileIdx] = navMesh->GetTileData(tileIdx);
        }
}

void CrowdNavigation::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    // Update streaming
    if (DV_INPUT->GetKeyPress(KEY_TAB))
    {
        useStreaming_ = !useStreaming_;
        toggle_streaming(useStreaming_);
    }

    if (useStreaming_)
        update_streaming();

}

void CrowdNavigation::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    if (draw_debug_)
    {
        // Visualize navigation mesh, obstacles and off-mesh connections
        scene_->GetComponent<DynamicNavigationMesh>()->draw_debug_geometry(true);
        // Visualize agents' path and position to reach
        scene_->GetComponent<CrowdManager>()->draw_debug_geometry(true);
    }
}

void CrowdNavigation::HandleCrowdAgentFailure(StringHash eventType, VariantMap& eventData)
{
    using namespace CrowdAgentFailure;

    auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());
    auto agentState = (CrowdAgentState)eventData[P_CROWD_AGENT_STATE].GetI32();

    // If the agent's state is invalid, likely from spawning on the side of a box, find a point in a larger area
    if (agentState == CA_STATE_INVALID)
    {
        // Get a point on the navmesh using more generous extents
        Vector3 newPos = scene_->GetComponent<DynamicNavigationMesh>()->FindNearestPoint(node->GetPosition(), Vector3(5.0f, 5.0f, 5.0f));
        // Set the new node position, CrowdAgent component will automatically reset the state of the agent
        node->SetPosition(newPos);
    }
}

void CrowdNavigation::HandleCrowdAgentReposition(StringHash eventType, VariantMap& eventData)
{
    static const char* WALKING_ANI = "models/jack_walk.ani";

    using namespace CrowdAgentReposition;

    auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());
    auto* agent = static_cast<CrowdAgent*>(eventData[P_CROWD_AGENT].GetPtr());
    Vector3 velocity = eventData[P_VELOCITY].GetVector3();
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Only Jack agent has animation controller
    auto* animCtrl = node->GetComponent<AnimationController>();
    if (animCtrl)
    {
        float speed = velocity.Length();
        if (animCtrl->IsPlaying(WALKING_ANI))
        {
            float speedRatio = speed / agent->GetMaxSpeed();
            // Face the direction of its velocity but moderate the turning speed based on the speed ratio and timeStep
            node->SetRotation(node->GetRotation().Slerp(Quaternion(Vector3::FORWARD, velocity), 10.0f * timeStep * speedRatio));
            // Throttle the animation speed based on agent speed ratio (ratio = 1 is full throttle)
            animCtrl->SetSpeed(WALKING_ANI, speedRatio * 1.5f);
        }
        else
            animCtrl->Play(WALKING_ANI, 0, true, 0.1f);

        // If speed is too low then stop the animation
        if (speed < agent->GetRadius())
            animCtrl->Stop(WALKING_ANI, 0.5f);
    }
}

void CrowdNavigation::HandleCrowdAgentFormation(StringHash eventType, VariantMap& eventData)
{
    using namespace CrowdAgentFormation;

    unsigned index = eventData[P_INDEX].GetU32();
    unsigned size = eventData[P_SIZE].GetU32();
    Vector3 position = eventData[P_POSITION].GetVector3();

    // The first agent will always move to the exact position, all other agents will select a random point nearby
    if (index)
    {
        auto* crowdManager = static_cast<CrowdManager*>(GetEventSender());
        auto* agent = static_cast<CrowdAgent*>(eventData[P_CROWD_AGENT].GetPtr());
        eventData[P_POSITION] = crowdManager->GetRandomPointInCircle(position, agent->GetRadius(), agent->GetQueryFilterType());
    }
}
