// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "vector2.h"

namespace dviglo
{

/// 2x2 matrix for rotation and scaling.
class DV_API Matrix2
{
public:
    /// Construct an identity matrix.
    Matrix2() noexcept :
        m00_(1.0f),
        m01_(0.0f),
        m10_(0.0f),
        m11_(1.0f)
    {
    }

    /// Copy-construct from another matrix.
    Matrix2(const Matrix2& matrix) noexcept = default;

    /// Construct from values.
    Matrix2(float v00, float v01,
            float v10, float v11) noexcept :
        m00_(v00),
        m01_(v01),
        m10_(v10),
        m11_(v11)
    {
    }

    /// Construct from a float array.
    explicit Matrix2(const float* data) noexcept :
        m00_(data[0]),
        m01_(data[1]),
        m10_(data[2]),
        m11_(data[3])
    {
    }

    /// Assign from another matrix.
    Matrix2& operator =(const Matrix2& rhs) noexcept = default;

    /// Test for equality with another matrix without epsilon.
    bool operator ==(const Matrix2& rhs) const
    {
        const float* leftData = Data();
        const float* rightData = rhs.Data();

        for (unsigned i = 0; i != 4; ++i)
        {
            if (leftData[i] != rightData[i])
                return false;
        }

        return true;
    }

    /// Test for inequality with another matrix without epsilon.
    bool operator !=(const Matrix2& rhs) const { return !(*this == rhs); }

    /// Multiply a Vector2.
    Vector2 operator *(const Vector2& rhs) const
    {
        return Vector2(
            m00_ * rhs.x + m01_ * rhs.y,
            m10_ * rhs.x + m11_ * rhs.y
        );
    }

    /// Add a matrix.
    Matrix2 operator +(const Matrix2& rhs) const
    {
        return Matrix2(
            m00_ + rhs.m00_,
            m01_ + rhs.m01_,
            m10_ + rhs.m10_,
            m11_ + rhs.m11_
        );
    }

    /// Subtract a matrix.
    Matrix2 operator -(const Matrix2& rhs) const
    {
        return Matrix2(
            m00_ - rhs.m00_,
            m01_ - rhs.m01_,
            m10_ - rhs.m10_,
            m11_ - rhs.m11_
        );
    }

    /// Multiply with a scalar.
    Matrix2 operator *(float rhs) const
    {
        return Matrix2(
            m00_ * rhs,
            m01_ * rhs,
            m10_ * rhs,
            m11_ * rhs
        );
    }

    /// Multiply a matrix.
    Matrix2 operator *(const Matrix2& rhs) const
    {
        return Matrix2(
            m00_ * rhs.m00_ + m01_ * rhs.m10_,
            m00_ * rhs.m01_ + m01_ * rhs.m11_,
            m10_ * rhs.m00_ + m11_ * rhs.m10_,
            m10_ * rhs.m01_ + m11_ * rhs.m11_
        );
    }

    /// Set scaling elements.
    void SetScale(const Vector2& scale)
    {
        m00_ = scale.x;
        m11_ = scale.y;
    }

    /// Set uniform scaling elements.
    void SetScale(float scale)
    {
        m00_ = scale;
        m11_ = scale;
    }

    /// Return the scaling part.
    Vector2 Scale() const
    {
        return Vector2(
            sqrtf(m00_ * m00_ + m10_ * m10_),
            sqrtf(m01_ * m01_ + m11_ * m11_)
        );
    }

    /// Return transpose.
    Matrix2 Transpose() const
    {
        return Matrix2(
            m00_,
            m10_,
            m01_,
            m11_
        );
    }

    /// Return scaled by a vector.
    Matrix2 Scaled(const Vector2& scale) const
    {
        return Matrix2(
            m00_ * scale.x,
            m01_ * scale.y,
            m10_ * scale.x,
            m11_ * scale.y
        );
    }

    /// Test for equality with another matrix with epsilon.
    bool Equals(const Matrix2& rhs) const
    {
        const float* leftData = Data();
        const float* rightData = rhs.Data();

        for (unsigned i = 0; i != 4; ++i)
        {
            if (!dviglo::Equals(leftData[i], rightData[i]))
                return false;
        }

        return true;
    }

    /// Return inverse.
    Matrix2 Inverse() const;

    /// Return float data.
    const float* Data() const { return &m00_; }

    /// Return whether any element is NaN.
    bool is_nan() const
    {
        const float* data = Data();
        for (unsigned i = 0; i < 4; ++i)
        {
            if (std::isnan(data[i]))
                return true;
        }
        return false;
    }

    /// Return whether any element is Inf.
    bool is_inf() const
    {
        const float* data = Data();
        for (unsigned i = 0; i < 4; ++i)
        {
            if (std::isinf(data[i]))
                return true;
        }
        return false;
    }

    /// Return as string.
    String ToString() const;

    float m00_;
    float m01_;
    float m10_;
    float m11_;

    /// Bulk transpose matrices.
    static void bulk_transpose(float* dest, const float* src, unsigned count)
    {
        for (unsigned i = 0; i < count; ++i)
        {
            dest[0] = src[0];
            dest[1] = src[2];
            dest[2] = src[1];
            dest[3] = src[3];

            dest += 4;
            src += 4;
        }
    }

    /// Zero matrix.
    static const Matrix2 ZERO;
    /// Identity matrix.
    static const Matrix2 IDENTITY;
};

/// Multiply a 2x2 matrix with a scalar.
inline Matrix2 operator *(float lhs, const Matrix2& rhs) { return rhs * lhs; }

}
