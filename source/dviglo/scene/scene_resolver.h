// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../containers/hash_map.h"
#include "../containers/ptr.h"

namespace dviglo
{

class Component;
class Node;

using ComponentId = id32;
using NodeId = id32;

/// Utility class that resolves node & component IDs after a scene or partial scene load.
class DV_API SceneResolver
{
public:
    /// Construct.
    SceneResolver();
    /// Destruct.
    ~SceneResolver();

    /// Reset. Clear all remembered nodes and components.
    void Reset();
    /// Remember a created node.
    void AddNode(NodeId oldID, Node* node);
    /// Remember a created component.
    void AddComponent(ComponentId oldID, Component* component);
    /// Resolve component and node ID attributes and reset.
    void Resolve();

private:
    /// Nodes.
    HashMap<NodeId, WeakPtr<Node>> nodes_;
    /// Components.
    HashMap<ComponentId, WeakPtr<Component>> components_;
};

}
