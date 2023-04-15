// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../graphics/graphics.h"
#include "../constant_buffer.h"
#include "../graphics_impl.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

using namespace std;

namespace dviglo
{

void ConstantBuffer::Release_OGL()
{
    if (gpu_object_name_)
    {
        if (GParams::is_headless())
            return;

#ifndef GL_ES_VERSION_2_0
        DV_GRAPHICS.SetUBO_OGL(0);
        glDeleteBuffers(1, &gpu_object_name_);
#endif
        gpu_object_name_ = 0;
    }

    shadowData_.reset();
    size_ = 0;
}

void ConstantBuffer::OnDeviceReset_OGL()
{
    if (size_)
        SetSize_OGL(size_); // Recreate
}

bool ConstantBuffer::SetSize_OGL(unsigned size)
{
    if (!size)
    {
        DV_LOGERROR("Can not create zero-sized constant buffer");
        return false;
    }

    // Round up to next 16 bytes
    size += 15;
    size &= 0xfffffff0;

    size_ = size;
    dirty_ = false;
    shadowData_ = make_unique<unsigned char[]>(size_);
    memset(shadowData_.get(), 0, size_);

    if (!GParams::is_headless())
    {
#ifndef GL_ES_VERSION_2_0
        if (!gpu_object_name_)
            glGenBuffers(1, &gpu_object_name_);
        DV_GRAPHICS.SetUBO_OGL(gpu_object_name_);
        glBufferData(GL_UNIFORM_BUFFER, size_, shadowData_.get(), GL_DYNAMIC_DRAW);
#endif
    }

    return true;
}

void ConstantBuffer::Apply_OGL()
{
    if (dirty_ && gpu_object_name_)
    {
#ifndef GL_ES_VERSION_2_0
        DV_GRAPHICS.SetUBO_OGL(gpu_object_name_);
        glBufferData(GL_UNIFORM_BUFFER, size_, shadowData_.get(), GL_DYNAMIC_DRAW);
#endif
        dirty_ = false;
    }
}

}
