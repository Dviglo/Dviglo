// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../graphics/drawable.h"
#include "../graphics_api/graphics_defs.h"
#include "urho_2d.h"

namespace dviglo
{

class Drawable2d;
class Renderer2D;
class Texture2D;
class VertexBuffer;

/// 2D vertex.
struct Vertex2D
{
    /// Position.
    Vector3 position_;
    /// Color.
    unsigned color_;
    /// UV.
    Vector2 uv_;
};

/// 2D source batch.
struct SourceBatch2D
{
    /// Construct.
    SourceBatch2D();

    /// Owner.
    WeakPtr<Drawable2d> owner_;
    /// Distance to camera.
    mutable float distance_;
    /// Draw order.
    int drawOrder_;
    /// Material.
    SharedPtr<Material> material_;
    /// Vertices.
    Vector<Vertex2D> vertices_;
};

/// Base class for 2D visible components.
class DV_API Drawable2d : public Drawable
{
    DV_OBJECT(Drawable2d);

public:
    /// Construct.
    explicit Drawable2d();
    /// Destruct.
    ~Drawable2d() override;
    /// Register object factory. Drawable must be registered first.
    static void register_object();

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set layer.
    void SetLayer(int layer);
    /// Set order in layer.
    void SetOrderInLayer(int orderInLayer);

    /// Return layer.
    int GetLayer() const { return layer_; }

    /// Return order in layer.
    int GetOrderInLayer() const { return orderInLayer_; }

    /// Return all source batches (called by Renderer2D).
    const Vector<SourceBatch2D>& GetSourceBatches();

protected:
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;
    /// Handle node transform being dirtied.
    void OnMarkedDirty(Node* node) override;
    /// Handle draw order changed.
    virtual void OnDrawOrderChanged() = 0;
    /// Update source batches.
    virtual void UpdateSourceBatches() = 0;

    /// Return draw order by layer and order in layer.
    int GetDrawOrder() const { return layer_ << 16u | orderInLayer_; }

    /// Layer.
    int layer_;
    /// Order in layer.
    int orderInLayer_;
    /// Source batches.
    Vector<SourceBatch2D> sourceBatches_;
    /// Source batches dirty flag.
    bool sourceBatchesDirty_;
    /// Renderer2D.
    WeakPtr<Renderer2D> renderer_;
};

}
