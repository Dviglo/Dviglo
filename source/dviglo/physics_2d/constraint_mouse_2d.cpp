// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "constraint_mouse_2d.h"
#include "physics_utils_2d.h"
#include "rigid_body_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;

ConstraintMouse2D::ConstraintMouse2D() :
    target_(Vector2::ZERO)
{
}

ConstraintMouse2D::~ConstraintMouse2D() = default;

void ConstraintMouse2D::register_object()
{
    DV_CONTEXT->RegisterFactory<ConstraintMouse2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Target", GetTarget, SetTarget, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Force", GetMaxForce, SetMaxForce, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Stiffness", GetStiffness, SetStiffness, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Damping", GetDamping, SetDamping, 0.0f, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintMouse2D::SetTarget(const Vector2& target)
{
    if (target == target_)
        return;

    target_ = target;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetTarget(ToB2Vec2(target));
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintMouse2D::SetMaxForce(float maxForce)
{
    if (maxForce == jointDef_.maxForce)
        return;

    jointDef_.maxForce = maxForce;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetMaxForce(maxForce);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintMouse2D::SetStiffness(float stiffness)
{
    if (stiffness == jointDef_.stiffness)
        return;

    jointDef_.stiffness = stiffness;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetStiffness(stiffness);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintMouse2D::SetDamping(float damping)
{
    if (damping == jointDef_.damping)
        return;

    jointDef_.damping = damping;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetDamping(damping);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

b2JointDef* ConstraintMouse2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = otherBody_->GetBody();
    b2Body* bodyB = ownerBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.bodyA = bodyA;
    jointDef_.bodyB = bodyB;
    jointDef_.collideConnected = collideConnected_;

    jointDef_.target = ToB2Vec2(target_);

    return &jointDef_;
}


bool ConstraintMouse2D::SetLinearStiffness(float frequencyHertz, float dampingRatio)
{
    if (!ownerBody_ || !otherBody_)
        return false;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return false;

    float stiffness, damping;
    b2LinearStiffness(stiffness, damping, frequencyHertz, dampingRatio, bodyA, bodyB);

    if (joint_)
    {
        static_cast<b2MouseJoint*>(joint_)->SetDamping(damping);
        static_cast<b2MouseJoint*>(joint_)->SetStiffness(stiffness);
    }
    else
    {
        RecreateJoint();
    }

    MarkNetworkUpdate();

    return true;
}

}
