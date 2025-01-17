// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "constraint_prismatic_2d.h"
#include "physics_utils_2d.h"
#include "rigid_body_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;

ConstraintPrismatic2D::ConstraintPrismatic2D() :
    anchor_(Vector2::ZERO),
    axis_(Vector2::RIGHT)
{
}

ConstraintPrismatic2D::~ConstraintPrismatic2D() = default;

void ConstraintPrismatic2D::register_object()
{
    DV_CONTEXT->RegisterFactory<ConstraintPrismatic2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Anchor", GetAnchor, SetAnchor, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Axis", GetAxis, SetAxis, Vector2::RIGHT, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Enable Limit", GetEnableLimit, SetEnableLimit, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Lower translation", GetLowerTranslation, SetLowerTranslation, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Upper translation", GetUpperTranslation, SetUpperTranslation, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Enable Motor", GetEnableMotor, SetEnableMotor, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Motor Force", GetMaxMotorForce, SetMaxMotorForce, 2.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Motor Speed", GetMotorSpeed, SetMotorSpeed, 0.7f, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintPrismatic2D::SetAnchor(const Vector2& anchor)
{
    if (anchor == anchor_)
        return;

    anchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetAxis(const Vector2& axis)
{
    if (axis == axis_)
        return;

    axis_ = axis;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetEnableLimit(bool enableLimit)
{
    if (enableLimit == jointDef_.enableLimit)
        return;

    jointDef_.enableLimit = enableLimit;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->EnableLimit(enableLimit);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetLowerTranslation(float lowerTranslation)
{
    if (lowerTranslation == jointDef_.lowerTranslation)
        return;

    jointDef_.lowerTranslation = lowerTranslation;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->SetLimits(lowerTranslation, jointDef_.upperTranslation);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetUpperTranslation(float upperTranslation)
{
    if (upperTranslation == jointDef_.upperTranslation)
        return;

    jointDef_.upperTranslation = upperTranslation;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->SetLimits(jointDef_.lowerTranslation, upperTranslation);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetEnableMotor(bool enableMotor)
{
    if (enableMotor == jointDef_.enableMotor)
        return;

    jointDef_.enableMotor = enableMotor;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->EnableMotor(enableMotor);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetMaxMotorForce(float maxMotorForce)
{
    if (maxMotorForce == jointDef_.maxMotorForce)
        return;

    jointDef_.maxMotorForce = maxMotorForce;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->SetMaxMotorForce(maxMotorForce);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintPrismatic2D::SetMotorSpeed(float motorSpeed)
{
    if (motorSpeed == jointDef_.motorSpeed)
        return;

    jointDef_.motorSpeed = motorSpeed;

    if (joint_)
        static_cast<b2PrismaticJoint*>(joint_)->SetMotorSpeed(motorSpeed);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

b2JointDef* ConstraintPrismatic2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB, ToB2Vec2(anchor_), ToB2Vec2(axis_));

    return &jointDef_;
}

}
