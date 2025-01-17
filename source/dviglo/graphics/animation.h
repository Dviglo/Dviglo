// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../containers/flag_set.h"
#include "../containers/ptr.h"
#include "../math/quaternion.h"
#include "../math/vector3.h"
#include "../resource/resource.h"

namespace dviglo
{

enum class AnimationChannels : u8
{
    None     = 0,
    Position = 1 << 0,
    Rotation = 1 << 1,
    Scale    = 1 << 2,
};
DV_FLAGS(AnimationChannels);

/// Skeletal animation keyframe.
struct AnimationKeyFrame
{
    /// Construct.
    AnimationKeyFrame() :
        time_(0.0f),
        scale_(Vector3::ONE)
    {
    }

    /// Keyframe time.
    float time_;
    /// Bone position.
    Vector3 position_;
    /// Bone rotation.
    Quaternion rotation_;
    /// Bone scale.
    Vector3 scale_;
};

/// Skeletal animation track, stores keyframes of a single bone.
struct DV_API AnimationTrack
{
    /// Construct.
    AnimationTrack()
    {
    }

    /// Assign keyframe at index.
    void SetKeyFrame(i32 index, const AnimationKeyFrame& keyFrame);
    /// Add a keyframe at the end.
    void AddKeyFrame(const AnimationKeyFrame& keyFrame);
    /// Insert a keyframe at index.
    void InsertKeyFrame(i32 index, const AnimationKeyFrame& keyFrame);
    /// Remove a keyframe at index.
    void RemoveKeyFrame(i32 index);
    /// Remove all keyframes.
    void RemoveAllKeyFrames();

    /// Return keyframe at index, or null if not found.
    AnimationKeyFrame* GetKeyFrame(i32 index);
    /// Return number of keyframes.
    i32 GetNumKeyFrames() const { return keyFrames_.Size(); }
    /// Return keyframe index based on time and previous index. Return false if animation is empty.
    bool GetKeyFrameIndex(float time, i32& index) const;

    /// Bone or scene node name.
    String name_;
    /// Name hash.
    StringHash nameHash_;
    /// Bitmask of included data (position, rotation, scale).
    AnimationChannels channelMask_{};
    /// Keyframes.
    Vector<AnimationKeyFrame> keyFrames_;
};

/// %Animation trigger point.
struct AnimationTriggerPoint
{
    /// Construct.
    AnimationTriggerPoint() :
        time_(0.0f)
    {
    }

    /// Trigger time.
    float time_;
    /// Trigger data.
    Variant data_;
};

/// Skeletal animation resource.
class DV_API Animation : public ResourceWithMetadata
{
    DV_OBJECT(Animation);

public:
    /// Construct.
    explicit Animation();
    /// Destruct.
    ~Animation() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;

    /// Set animation name.
    void SetAnimationName(const String& name);
    /// Set animation length.
    void SetLength(float length);
    /// Create and return a track by name. If track by same name already exists, returns the existing.
    AnimationTrack* CreateTrack(const String& name);
    /// Remove a track by name. Return true if was found and removed successfully. This is unsafe if the animation is currently used in playback.
    bool RemoveTrack(const String& name);
    /// Remove all tracks. This is unsafe if the animation is currently used in playback.
    void RemoveAllTracks();
    /// Set a trigger point at index.
    void SetTrigger(i32 index, const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(float time, bool timeIsNormalized, const Variant& data);
    /// Remove a trigger point by index.
    void RemoveTrigger(i32 index);
    /// Remove all trigger points.
    void RemoveAllTriggers();
    /// Resize trigger point vector.
    void SetNumTriggers(i32 num);
    /// Clone the animation.
    SharedPtr<Animation> Clone(const String& cloneName = String::EMPTY) const;

    /// Return animation name.
    const String& GetAnimationName() const { return animationName_; }

    /// Return animation name hash.
    StringHash GetAnimationNameHash() const { return animationNameHash_; }

    /// Return animation length.
    float GetLength() const { return length_; }

    /// Return all animation tracks.
    const HashMap<StringHash, AnimationTrack>& GetTracks() const { return tracks_; }

    /// Return number of animation tracks.
    i32 GetNumTracks() const { return tracks_.Size(); }

    /// Return animation track by index.
    AnimationTrack *GetTrack(i32 index);

    /// Return animation track by name.
    AnimationTrack* GetTrack(const String& name);
    /// Return animation track by name hash.
    AnimationTrack* GetTrack(StringHash nameHash);

    /// Return animation trigger points.
    const Vector<AnimationTriggerPoint>& GetTriggers() const { return triggers_; }

    /// Return number of animation trigger points.
    i32 GetNumTriggers() const { return triggers_.Size(); }

    /// Return a trigger point by index.
    AnimationTriggerPoint* GetTrigger(i32 index);

private:
    /// Animation name.
    String animationName_;
    /// Animation name hash.
    StringHash animationNameHash_;
    /// Animation length.
    float length_;
    /// Animation tracks.
    HashMap<StringHash, AnimationTrack> tracks_;
    /// Animation trigger points.
    Vector<AnimationTriggerPoint> triggers_;
};

}
