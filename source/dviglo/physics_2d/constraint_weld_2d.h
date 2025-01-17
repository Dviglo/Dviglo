// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "constraint_2d.h"

namespace dviglo
{

/// 2D weld constraint component.
class DV_API ConstraintWeld2D : public Constraint2D
{
    DV_OBJECT(ConstraintWeld2D);

public:
    /// Construct.
    explicit ConstraintWeld2D();

    /// Destruct.
    ~ConstraintWeld2D() override;

    /// Register object factory.
    static void register_object();

    /// Set anchor.
    void SetAnchor(const Vector2& anchor);

    /// Return anchor.
    const Vector2& GetAnchor() const { return anchor_; }

    /// Set linear stiffness in N/m.
    void SetStiffness(float stiffness);

    /// Return linear stiffness in N/m.
    float GetStiffness() const { return jointDef_.stiffness; }

    /// Set linear damping in N*s/m.
    void SetDamping(float damping);

    /// Return linear damping in N*s/m.
    float GetDamping() const { return jointDef_.damping; }

    /// Calc and set stiffness and damping. Must be used after set owner and other bodies.
    bool SetAngularStiffness(float frequencyHertz, float dampingRatio);

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2WeldJointDef jointDef_;

    /// Anchor.
    Vector2 anchor_;
};

}
