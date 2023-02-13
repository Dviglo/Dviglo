// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

namespace dviglo
{

class HashBase;
class ListBase;
class String;
class VectorBase;

/// Swap two values.
template <class T> inline void Swap(T& first, T& second)
{
    T temp = first;
    first = second;
    second = temp;
}

template <> URHO3D_API void Swap<String>(String& first, String& second);
template <> URHO3D_API void Swap<VectorBase>(VectorBase& first, VectorBase& second);
template <> URHO3D_API void Swap<ListBase>(ListBase& first, ListBase& second);
template <> URHO3D_API void Swap<HashBase>(HashBase& first, HashBase& second);

}
