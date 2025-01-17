// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

/// Multiple viewports example.
/// This sample demonstrates:
///     - Setting up two viewports with two separate cameras
///     - Adding post processing effects to a viewport's render path and toggling them
class MultipleViewports : public Sample
{
    DV_OBJECT(MultipleViewports);

public:
    /// Construct.
    explicit MultipleViewports();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up viewports.
    void SetupViewports();
    /// Subscribe to application-wide logic update and post-render update events.
    void subscribe_to_events();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void handle_post_render_update(StringHash eventType, VariantMap& eventData);

    /// Rear-facing camera scene node.
    SharedPtr<Node> rearCameraNode_;
    /// Flag for drawing debug geometry.
    bool draw_debug_;
};
