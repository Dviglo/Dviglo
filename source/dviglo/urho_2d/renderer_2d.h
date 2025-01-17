// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../graphics/drawable.h"
#include "../math/frustum.h"

namespace dviglo
{

class Drawable2d;
class IndexBuffer;
class Material;
class Technique;
class Texture2D;
class VertexBuffer;
struct FrameInfo;
struct SourceBatch2D;

/// 2D view batch info.
struct ViewBatchInfo2D
{
    /// Construct.
    ViewBatchInfo2D();

    /// Vertex buffer update frame number.
    i32 vertexBufferUpdateFrameNumber_;
    /// Index count.
    unsigned indexCount_;
    /// Vertex count.
    unsigned vertexCount_;
    /// Vertex buffer.
    std::shared_ptr<VertexBuffer> vertexBuffer_;
    /// Batch updated frame number.
    i32 batchUpdatedFrameNumber_;
    /// Source batches.
    Vector<const SourceBatch2D*> sourceBatches_;
    /// Batch count.
    unsigned batchCount_;
    /// Distances.
    Vector<float> distances_;
    /// Materials.
    Vector<SharedPtr<Material>> materials_;
    /// Geometries.
    Vector<SharedPtr<Geometry>> geometries_;
};

/// 2D renderer component.
class DV_API Renderer2D : public Drawable
{
    DV_OBJECT(Renderer2D);

    friend void CheckDrawableVisibilityWork(const WorkItem* item, i32 threadIndex);

public:
    /// Construct.
    explicit Renderer2D();
    /// Destruct.
    ~Renderer2D() override;
    /// Register object factory.
    static void register_object();

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, Vector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void update_batches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update).
    void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    UpdateGeometryType GetUpdateGeometryType() override;

    /// Add Drawable2d.
    void AddDrawable(Drawable2d* drawable);
    /// Remove Drawable2d.
    void RemoveDrawable(Drawable2d* drawable);
    /// Return material by texture and blend mode.
    Material* GetMaterial(Texture2D* texture, BlendMode blendMode);

    /// Check visibility.
    bool CheckVisibility(Drawable2d* drawable) const;

private:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Create material by texture and blend mode.
    SharedPtr<Material> CreateMaterial(Texture2D* texture, BlendMode blendMode);
    /// Handle view update begin event. Determine Drawable2d's and their batches here.
    void HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData);
    /// Get all drawables in node.
    void GetDrawables(Vector<Drawable2d*>& drawables, Node* node);
    /// Update view batch info.
    void UpdateViewBatchInfo(ViewBatchInfo2D& viewBatchInfo, Camera* camera);
    /// Add view batch.
    void AddViewBatch(ViewBatchInfo2D& viewBatchInfo, Material* material,
        unsigned indexStart, unsigned indexCount, unsigned vertexStart, unsigned vertexCount, float distance);

    /// Index buffer.
    std::shared_ptr<IndexBuffer> indexBuffer_;
    /// Material.
    SharedPtr<Material> material_;
    /// Drawables.
    Vector<Drawable2d*> drawables_;
    /// View frame info for current frame.
    FrameInfo frame_;
    /// View batch info.
    HashMap<Camera*, ViewBatchInfo2D> viewBatchInfos_;
    /// Frustum for current frame.
    Frustum frustum_;
    /// View mask of current camera for visibility checking.
    unsigned viewMask_;
    /// Cached materials.
    HashMap<Texture2D*, HashMap<int, SharedPtr<Material>>> cachedMaterials_;
    /// Cached techniques per blend mode.
    HashMap<int, SharedPtr<Technique>> cachedTechniques_;
};

}
