// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../dviglo_config.h"

#include "../container/allocator.h"
#include "../container/swap.h"

namespace dviglo
{

/// Doubly-linked list node base class.
struct ListNodeBase
{
    /// Construct.
    ListNodeBase() :
        prev_(nullptr),
        next_(nullptr)
    {
    }

    /// Previous node.
    ListNodeBase* prev_;
    /// Next node.
    ListNodeBase* next_;
};

/// Doubly-linked list iterator base class.
struct ListIteratorBase
{
    /// Construct.
    ListIteratorBase() :
        ptr_(nullptr)
    {
    }

    /// Construct with a node pointer.
    explicit ListIteratorBase(ListNodeBase* ptr) :
        ptr_(ptr)
    {
    }

    /// Test for equality with another iterator.
    bool operator ==(const ListIteratorBase& rhs) const { return ptr_ == rhs.ptr_; }

    /// Test for inequality with another iterator.
    bool operator !=(const ListIteratorBase& rhs) const { return ptr_ != rhs.ptr_; }

    /// Go to the next node.
    void GotoNext()
    {
        if (ptr_)
            ptr_ = ptr_->next_;
    }

    /// Go to the previous node.
    void GotoPrev()
    {
        if (ptr_)
            ptr_ = ptr_->prev_;
    }

    /// Node pointer.
    ListNodeBase* ptr_;
};

/// Doubly-linked list base class.
class URHO3D_API ListBase
{
public:
    /// Construct.
    ListBase() :
        head_(nullptr),
        tail_(nullptr),
        allocator_(nullptr),
        size_(0)
    {
    }

    /// Swap with another linked list.
    void Swap(ListBase& rhs)
    {
        dviglo::Swap(head_, rhs.head_);
        dviglo::Swap(tail_, rhs.tail_);
        dviglo::Swap(allocator_, rhs.allocator_);
        dviglo::Swap(size_, rhs.size_);
    }

protected:
    /// Head node pointer.
    ListNodeBase* head_;
    /// Tail node pointer.
    ListNodeBase* tail_;
    /// Node allocator.
    AllocatorBlock* allocator_;
    /// Number of nodes.
    i32 size_;
};

}
