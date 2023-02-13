// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../io/file.h"

#include <SDL3/SDL_rwops.h>

// TODO: После обновления SDL до 3й версии поменялись функции Read и Write. Они нужны для Gestures

namespace dviglo
{

/// Template wrapper class for using Serializer / Deserializer or their subclasses through SDL's RWOps structure.
template <class T> class RWOpsWrapper
{
public:
    /// Construct with object reference.
    explicit RWOpsWrapper(T& object)
    {
        ops_.type = dynamic_cast<File*>(&object) ? SDL_RWOPS_STDFILE : SDL_RWOPS_MEMORY;
        ops_.hidden.unknown.data1 = &object;
        ops_.size = &Size;
        ops_.seek = &Seek;
        ops_.close = &Close;
        ops_.read = &Read;
        ops_.write = &Write;
    }

    /// Return the RWOps structure.
    SDL_RWops* GetRWOps() { return &ops_; }

private:
    /// Return data size of the object.
    static Sint64 Size(SDL_RWops* context)
    {
        auto* object = reinterpret_cast<T*>(context->hidden.unknown.data1);
        auto* des = dynamic_cast<Deserializer*>(object);
        return des ? (Sint64)des->GetSize() : 0;
    }

    /// Seek within the object's data.
    static Sint64 Seek(SDL_RWops* context, Sint64 offset, int whence)
    {
        auto* object = reinterpret_cast<T*>(context->hidden.unknown.data1);
        auto* des = dynamic_cast<Deserializer*>(object);
        if (!des)
            return 0;

        switch (whence)
        {
        case SDL_RW_SEEK_SET:
            des->Seek((unsigned)offset);
            break;

        case SDL_RW_SEEK_CUR:
            des->Seek((unsigned)(des->GetPosition() + offset));
            break;

        case SDL_RW_SEEK_END:
            des->Seek((unsigned)(des->GetSize() + offset));
            break;

        default:
            assert(false);  // Should never reach here
            break;
        }

        return (Sint64)des->GetPosition();
    }

    /// Close the object. Only meaningful for files, no-op otherwise.
    static int Close(SDL_RWops* context)
    {
        auto* object = reinterpret_cast<T*>(context->hidden.unknown.data1);
        auto* file = dynamic_cast<File*>(object);
        if (file)
            file->Close();
        return 0;
    }

    /// Read from the object. Return number of "packets" read.
    static Sint64 Read(SDL_RWops* context, void* ptr, Sint64 size)
    {
        // TODO: сделать
        //auto* object = reinterpret_cast<T*>(context->hidden.unknown.data1);
        //auto* des = dynamic_cast<Deserializer*>(object);
        //return des ? (size_t)(des->Read(ptr, (unsigned)(size * maxNum)) / size) : 0;
        return 0;
    }

    /// Write to the object. Return number of "packets" written.
    static Sint64 Write(SDL_RWops* context, const void* ptr, Sint64 size)
    {
        // TODO: Сделать
        //auto* object = reinterpret_cast<T*>(context->hidden.unknown.data1);
        //auto* ser = dynamic_cast<Serializer*>(object);
        //return ser ? (size_t)(ser->Write(ptr, (unsigned)(size * maxNum)) / size) : 0;
        return 0;
    }

    /// SDL RWOps structure associated with the object.
    SDL_RWops ops_;
};

}
