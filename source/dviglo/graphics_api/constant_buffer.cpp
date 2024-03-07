// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../graphics/graphics.h"
#include "constant_buffer.h"
#include "../io/log.h"

#include "../common/debug_new.h"

namespace dviglo
{

ConstantBuffer::ConstantBuffer()
{
}

ConstantBuffer::~ConstantBuffer()
{
    Release();
}

void ConstantBuffer::SetParameter(unsigned offset, unsigned size, const void* data)
{
    if (offset + size > size_)
        return; // Would overflow the buffer

    memcpy(&shadowData_[offset], data, size);
    dirty_ = true;
}

void ConstantBuffer::SetVector3ArrayParameter(unsigned offset, unsigned rows, const void* data)
{
    if (offset + rows * 4 * sizeof(float) > size_)
        return; // Would overflow the buffer

    auto* dest = (float*)&shadowData_[offset];
    const auto* src = (const float*)data;

    while (rows--)
    {
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        ++dest; // Skip over the w coordinate
    }

    dirty_ = true;
}

void ConstantBuffer::Release()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif
}

void ConstantBuffer::OnDeviceReset()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif
}

bool ConstantBuffer::SetSize(unsigned size)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetSize_OGL(size);
#endif

    return {}; // Prevent warning
}

void ConstantBuffer::Apply()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Apply_OGL();
#endif
}

}
