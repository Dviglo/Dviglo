// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../physics_2d/constraint_2d.h"

namespace Urho3D
{

/// 2D revolute constraint component.
class URHO3D_API ConstraintRevolute2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintRevolute2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintRevolute2D(Context* context);
    /// Destruct.
    ~ConstraintRevolute2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set anchor.
    void SetAnchor(const Vector2& anchor);
    /// Set enable limit.
    void SetEnableLimit(bool enableLimit);
    /// Set lower angle.
    void SetLowerAngle(float lowerAngle);
    /// Set upper angle.
    void SetUpperAngle(float upperAngle);
    /// Set enable motor.
    void SetEnableMotor(bool enableMotor);
    /// Set motor speed.
    void SetMotorSpeed(float motorSpeed);
    /// Set max motor torque.
    void SetMaxMotorTorque(float maxMotorTorque);

    /// Return anchor.
    const Vector2& GetAnchor() const { return anchor_; }

    /// Return enable limit.
    bool GetEnableLimit() const { return jointDef_.enableLimit; }

    /// Return lower angle.
    float GetLowerAngle() const { return jointDef_.lowerAngle; }

    /// Return upper angle.
    float GetUpperAngle() const { return jointDef_.upperAngle; }

    /// Return enable motor.
    bool GetEnableMotor() const { return jointDef_.enableMotor; }

    /// Return motor speed.
    float GetMotorSpeed() const { return jointDef_.motorSpeed; }

    /// Return max motor torque.
    float GetMaxMotorTorque() const { return jointDef_.maxMotorTorque; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2RevoluteJointDef jointDef_;
    /// Anchor.
    Vector2 anchor_;
};

}
