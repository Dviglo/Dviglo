// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../physics_2d/constraint_2d.h"

namespace Urho3D
{

/// 2D motor constraint component.
class URHO3D_API ConstraintMotor2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintMotor2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintMotor2D(Context* context);
    /// Destruct.
    ~ConstraintMotor2D() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set linear offset.
    void SetLinearOffset(const Vector2& linearOffset);
    /// Set angular offset.
    void SetAngularOffset(float angularOffset);
    /// Set max force.
    void SetMaxForce(float maxForce);
    /// Set max torque.
    void SetMaxTorque(float maxTorque);
    /// Set correction factor.
    void SetCorrectionFactor(float correctionFactor);

    /// Return linear offset.
    const Vector2& GetLinearOffset() const { return linearOffset_; }

    /// Return angular offset.
    float GetAngularOffset() const { return jointDef_.angularOffset; }

    /// Return max force.
    float GetMaxForce() const { return jointDef_.maxForce; }

    /// Return max torque.
    float GetMaxTorque() const { return jointDef_.maxTorque; }

    /// Return correction factor.
    float GetCorrectionFactor() const { return jointDef_.correctionFactor; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2MotorJointDef jointDef_;
    /// Linear offset.
    Vector2 linearOffset_;
};

}
