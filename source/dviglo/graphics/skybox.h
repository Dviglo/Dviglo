// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "static_model.h"

namespace dviglo
{

/// Static model component with fixed position in relation to the camera.
class DV_API Skybox : public StaticModel
{
    DV_OBJECT(Skybox);

public:
    /// Construct.
    explicit Skybox();
    /// Destruct.
    ~Skybox() override;
    /// Register object factory. StaticModel must be registered first.
    static void register_object();

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, Vector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void update_batches(const FrameInfo& frame) override;

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;

    /// Custom world transform per camera.
    HashMap<Camera*, Matrix3x4> customWorldTransforms_;
    /// Last frame counter for knowing when to erase the custom world transforms of previous frame.
    unsigned lastFrame_;
};

}
