// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>
#include <dviglo/core/context.h>
#include <dviglo/io/memory_buffer.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>

#include <dviglo/common/debug_new.h>

#include "Mover.h"

Mover::Mover() :
    speed_(0.8f),
    currentPathID_(1),
    emitTime_(0.0f),
    fightTimer_(0.0f),
    flip_(0.0f)
{
    // Only the scene update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::Update);
}

void Mover::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<Mover>();

    // These macros register the class attribute to the Context for automatic load / save handling.
    // We specify the Default attribute mode which means it will be used both for saving into file, and network replication.
    DV_ACCESSOR_ATTRIBUTE("Path", GetPathAttr, SetPathAttr, Variant::emptyBuffer, AM_DEFAULT);
    DV_ATTRIBUTE("Speed", speed_, 0.8f, AM_DEFAULT);
    DV_ATTRIBUTE("Current Path ID", currentPathID_, 1, AM_DEFAULT);
    DV_ATTRIBUTE("Emit Time", emitTime_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Fight Timer", fightTimer_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Flip Animation", flip_, 0.0f, AM_DEFAULT);
}

void Mover::SetPathAttr(const Vector<byte>& value)
{
    if (value.Empty())
        return;

    MemoryBuffer buffer(value);
    while (!buffer.IsEof())
        path_.Push(buffer.ReadVector2());
}

Vector<byte> Mover::GetPathAttr() const
{
    VectorBuffer buffer;

    for (const Vector2& point : path_)
        buffer.WriteVector2(point);

    return buffer.GetBuffer();
}

void Mover::Update(float timeStep)
{
    if (path_.Size() < 2)
        return;

    // Handle Orc states (idle/wounded/fighting)
    if (node_->GetName() == "Orc")
    {
        auto* animatedSprite = node_->GetComponent<AnimatedSprite2D>();
        String anim = "run";

        // Handle wounded state
        if (emitTime_ > 0.0f)
        {
            emitTime_ += timeStep;
            anim = "dead";

            // Handle dead
            if (emitTime_ >= 3.0f)
            {
                node_->Remove();
                return;
            }
        }
        else
        {
            // Handle fighting state
            if (fightTimer_ > 0.0f)
            {
                anim = "attack";
                flip_ = GetScene()->GetChild("Imp", true)->GetPosition().x_ - node_->GetPosition().x_;
                fightTimer_ += timeStep;
                if (fightTimer_ >= 3.0f)
                    fightTimer_ = 0.0f; // Reset
            }
            // Flip Orc animation according to speed, or player position when fighting
            animatedSprite->SetFlipX(flip_ >= 0.0f);
        }
        // Animate
        if (animatedSprite->GetAnimation() != anim)
            animatedSprite->SetAnimation(anim);
    }

    // Don't move if fighting or wounded
    if (fightTimer_ > 0.0f || emitTime_ > 0.0f)
        return;

    // Set direction and move to target
    Vector2 dir = path_[currentPathID_] - node_->GetPosition2D();
    Vector2 dirNormal = dir.Normalized();
    node_->Translate(Vector3(dirNormal.x_, dirNormal.y_, 0.0f) * Abs(speed_) * timeStep);
    flip_ = dir.x_;

    // Check for new target to reach
    if (Abs(dir.Length()) < 0.1f)
    {
        if (speed_ > 0.0f)
        {
            if (currentPathID_ + 1 < path_.Size())
                currentPathID_ = currentPathID_ + 1;
            else
            {
                // If loop, go to first waypoint, which equates to last one (and never reverse)
                if (path_[currentPathID_] == path_[0])
                {
                    currentPathID_ = 1;
                    return;
                }
                // Reverse path if not looping
                currentPathID_ = currentPathID_ - 1;
                speed_ = -speed_;
            }
        }
        else
        {
            if (currentPathID_ - 1 >= 0)
                currentPathID_ = currentPathID_ - 1;
            else
            {
                currentPathID_ = 1;
                speed_ = -speed_;
            }
        }
    }
}
