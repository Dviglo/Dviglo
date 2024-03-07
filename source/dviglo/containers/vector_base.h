// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

#include "iter.h"

namespace dviglo
{

/// %Vector base class.
/** Note that to prevent extra memory use due to vtable pointer, %VectorBase intentionally does not declare a virtual destructor
    and therefore %VectorBase pointers should never be used.
  */
class DV_API VectorBase
{
public:
    /// Construct.
    VectorBase() noexcept :
        size_(0),
        capacity_(0),
        buffer_(nullptr)
    {
    }

    /// Swap with another vector.
    void Swap(VectorBase& rhs)
    {
        std::swap(size_, rhs.size_);
        std::swap(capacity_, rhs.capacity_);
        std::swap(buffer_, rhs.buffer_);
    }

protected:
    static u8* AllocateBuffer(i32 size);

    /// Size of vector.
    i32 size_;
    /// Buffer capacity.
    i32 capacity_;
    /// Buffer.
    u8* buffer_;
};

}
