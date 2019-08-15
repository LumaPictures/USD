//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
////////////////////////////////////////////////////////////////////////
// This file is generated by a script.  Do not edit directly.  Edit the
// vec.template.h file to make changes.

#ifndef GF_VEC3F_H
#define GF_VEC3F_H

/// \file gf/vec3f.h
/// \ingroup group_gf_LinearAlgebra

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/gf/limits.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/math.h"

#include <boost/functional/hash.hpp>

#include <cstddef>
#include <cmath>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class GfVec3f;

template <>
struct GfIsGfVec<class GfVec3f> { static const bool value = true; };

/// \class GfVec3f
/// \ingroup group_gf_LinearAlgebra
///
/// Basic type for a vector of 3 float components.
///
/// Represents a vector of 3 components of type \c float.
/// It is intended to be fast and simple.
///
class GfVec3f
{
public:
    /// Scalar element type and dimension.
    typedef float ScalarType;
    static const size_t dimension = 3;

    /// Default constructor does no initialization.
    GfVec3f() = default;

    /// Initialize all elements to a single value.
    explicit GfVec3f(float value) {
        _data[0] = value;
        _data[1] = value;
        _data[2] = value;
    }

    /// Initialize all elements with explicit arguments.
    GfVec3f(float s0, float s1, float s2) {
        Set(s0, s1, s2);
    }

    /// Construct with pointer to values.
    template <class Scl>
    explicit GfVec3f(Scl const *p) { Set(p); }

    /// Construct from GfVec3d.
    explicit GfVec3f(class GfVec3d const &other);

    /// Implicitly convert from GfVec3h.
    GfVec3f(class GfVec3h const &other);

    /// Implicitly convert from GfVec3i.
    GfVec3f(class GfVec3i const &other);
 
    /// Create a unit vector along the X-axis.
    static GfVec3f XAxis() {
        GfVec3f result(0);
        result[0] = 1;
        return result;
    }
    /// Create a unit vector along the Y-axis.
    static GfVec3f YAxis() {
        GfVec3f result(0);
        result[1] = 1;
        return result;
    }
    /// Create a unit vector along the Z-axis.
    static GfVec3f ZAxis() {
        GfVec3f result(0);
        result[2] = 1;
        return result;
    }

    /// Create a unit vector along the i-th axis, zero-based.  Return the zero
    /// vector if \p i is greater than or equal to 3.
    static GfVec3f Axis(size_t i) {
        GfVec3f result(0);
        if (i < 3)
            result[i] = 1;
        return result;
    }

    /// Set all elements with passed arguments.
    GfVec3f &Set(float s0, float s1, float s2) {
        _data[0] = s0;
        _data[1] = s1;
        _data[2] = s2;
        return *this;
    }

    /// Set all elements with a pointer to data.
    GfVec3f &Set(float const *a) {
        return Set(a[0], a[1], a[2]);
    }

    /// Direct data access.
    float const *data() const { return _data; }
    float *data() { return _data; }
    float const *GetArray() const { return data(); }

    /// Indexing.
    float const &operator[](size_t i) const { return _data[i]; }
    float &operator[](size_t i) { return _data[i]; }

    /// Hash.
    friend inline size_t hash_value(GfVec3f const &vec) {
        size_t h = 0;
        boost::hash_combine(h, vec[0]);
        boost::hash_combine(h, vec[1]);
        boost::hash_combine(h, vec[2]);
        return h;
    }

    /// Equality comparison.
    bool operator==(GfVec3f const &other) const {
        return _data[0] == other[0] &&
               _data[1] == other[1] &&
               _data[2] == other[2];
    }
    bool operator!=(GfVec3f const &other) const {
        return !(*this == other);
    }

    // TODO Add inequality for other vec types...
    /// Equality comparison.
    GF_API
    bool operator==(class GfVec3d const &other) const;
    /// Equality comparison.
    GF_API
    bool operator==(class GfVec3h const &other) const;
    /// Equality comparison.
    GF_API
    bool operator==(class GfVec3i const &other) const;
    
    /// Create a vec with negated elements.
    GfVec3f operator-() const {
        return GfVec3f(-_data[0], -_data[1], -_data[2]);
    }

    /// Addition.
    GfVec3f &operator+=(GfVec3f const &other) {
        _data[0] += other[0];
        _data[1] += other[1];
        _data[2] += other[2];
        return *this;
    }
    friend GfVec3f operator+(GfVec3f const &l, GfVec3f const &r) {
        return GfVec3f(l) += r;
    }

    /// Subtraction.
    GfVec3f &operator-=(GfVec3f const &other) {
        _data[0] -= other[0];
        _data[1] -= other[1];
        _data[2] -= other[2];
        return *this;
    }
    friend GfVec3f operator-(GfVec3f const &l, GfVec3f const &r) {
        return GfVec3f(l) -= r;
    }

    /// Multiplication by scalar.
    GfVec3f &operator*=(double s) {
        _data[0] *= s;
        _data[1] *= s;
        _data[2] *= s;
        return *this;
    }
    GfVec3f operator*(double s) const {
        return GfVec3f(*this) *= s;
    }
    friend GfVec3f operator*(double s, GfVec3f const &v) {
        return v * s;
    }

        /// Division by scalar.
    // TODO should divide by the scalar type.
    GfVec3f &operator/=(double s) {
        // TODO This should not multiply by 1/s, it should do the division.
        // Doing the division is more numerically stable when s is close to
        // zero.
        return *this *= (1.0 / s);
    }
    GfVec3f operator/(double s) const {
        return *this * (1.0 / s);
    }
    
    /// See GfDot().
    float operator*(GfVec3f const &v) const {
        return _data[0] * v[0] + _data[1] * v[1] + _data[2] * v[2];
    }

    /// Returns the projection of \p this onto \p v. That is:
    /// \code
    /// v * (*this * v)
    /// \endcode
    GfVec3f GetProjection(GfVec3f const &v) const {
        return v * (*this * v);
    }

    /// Returns the orthogonal complement of \p this->GetProjection(b).
    /// That is:
    /// \code
    ///  *this - this->GetProjection(b)
    /// \endcode
    GfVec3f GetComplement(GfVec3f const &b) const {
        return *this - this->GetProjection(b);
    }

    /// Squared length.
    float GetLengthSq() const {
        return *this * *this;
    }

    /// Length
    float GetLength() const {
        // TODO should use GfSqrt.
        return sqrt(GetLengthSq());
    }

    /// Normalizes the vector in place to unit length, returning the
    /// length before normalization. If the length of the vector is
    /// smaller than \p eps, then the vector is set to vector/\c eps.
    /// The original length of the vector is returned. See also GfNormalize().
    ///
    /// \todo This was fixed for bug 67777. This is a gcc64 optimizer bug.
    /// By tickling the code, it no longer tries to write into
    /// an illegal memory address (in the code section of memory).
    float Normalize(float eps = GF_MIN_VECTOR_LENGTH) {
        // TODO this seems suspect...  suggest dividing by length so long as
        // length is not zero.
        float length = GetLength();
        *this /= (length > eps) ? length : eps;
        return length;
    }

    GfVec3f GetNormalized(float eps = GF_MIN_VECTOR_LENGTH) const {
        GfVec3f normalized(*this);
        normalized.Normalize(eps);
        return normalized;
    }

    /// Orthogonalize and optionally normalize a set of basis vectors. This
    /// uses an iterative method that is very stable even when the vectors are
    /// far from orthogonal (close to colinear).  The number of iterations and
    /// thus the computation time does increase as the vectors become close to
    /// colinear, however. Returns a bool specifying whether the solution
    /// converged after a number of iterations.  If it did not converge, the
    /// returned vectors will be as close as possible to orthogonal within the
    /// iteration limit. Colinear vectors will be unaltered, and the method
    /// will return false.
    GF_API
    static bool OrthogonalizeBasis(
        GfVec3f *tx, GfVec3f *ty, GfVec3f *tz,
        const bool normalize,
        double eps = GF_MIN_ORTHO_TOLERANCE);

    /// Sets \c v1 and \c v2 to unit vectors such that v1, v2 and *this are
    /// mutually orthogonal.  If the length L of *this is smaller than \c eps,
    /// then v1 and v2 will have magnitude L/eps.  As a result, the function
    /// delivers a continuous result as *this shrinks in length.
    GF_API
    void BuildOrthonormalFrame(GfVec3f *v1, GfVec3f *v2,
                    float eps = GF_MIN_VECTOR_LENGTH) const;

  
private:
    float _data[3];
};

/// Output a GfVec3f.
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream &, GfVec3f const &);


PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE

inline
GfVec3f::GfVec3f(class GfVec3d const &other)
{
    _data[0] = other[0];
    _data[1] = other[1];
    _data[2] = other[2];
}
inline
GfVec3f::GfVec3f(class GfVec3h const &other)
{
    _data[0] = other[0];
    _data[1] = other[1];
    _data[2] = other[2];
}
inline
GfVec3f::GfVec3f(class GfVec3i const &other)
{
    _data[0] = other[0];
    _data[1] = other[1];
    _data[2] = other[2];
}

/// Returns component-wise multiplication of vectors \p v1 and \p v2.
inline GfVec3f
GfCompMult(GfVec3f const &v1, GfVec3f const &v2) {
    return GfVec3f(
        v1[0] * v2[0],
        v1[1] * v2[1],
        v1[2] * v2[2]
        );
}

/// Returns component-wise quotient of vectors \p v1 and \p v2.
inline GfVec3f
GfCompDiv(GfVec3f const &v1, GfVec3f const &v2) {
    return GfVec3f(
        v1[0] / v2[0],
        v1[1] / v2[1],
        v1[2] / v2[2]
        );
}

/// Returns the dot (inner) product of two vectors.
inline float
GfDot(GfVec3f const &v1, GfVec3f const &v2) {
    return v1 * v2;
}


/// Returns the geometric length of \c v.
inline float
GfGetLength(GfVec3f const &v)
{
    return v.GetLength();
}

/// Normalizes \c *v in place to unit length, returning the length before
/// normalization. If the length of \c *v is smaller than \p eps then \c *v is
/// set to \c *v/eps.  The original length of \c *v is returned.
inline float
GfNormalize(GfVec3f *v, float eps = GF_MIN_VECTOR_LENGTH)
{
    return v->Normalize(eps);
}

/// Returns a normalized (unit-length) vector with the same direction as \p v.
/// If the length of this vector is smaller than \p eps, the vector divided by
/// \p eps is returned.
inline GfVec3f
GfGetNormalized(GfVec3f const &v, float eps = GF_MIN_VECTOR_LENGTH)
{
    return v.GetNormalized(eps);
}

/// Returns the projection of \p a onto \p b. That is:
/// \code
/// b * (a * b)
/// \endcode
inline GfVec3f
GfGetProjection(GfVec3f const &a, GfVec3f const &b)
{
    return a.GetProjection(b);
}

/// Returns the orthogonal complement of \p a.GetProjection(b). That is:
/// \code
///  a - a.GetProjection(b)
/// \endcode
inline GfVec3f
GfGetComplement(GfVec3f const &a, GfVec3f const &b)
{
    return a.GetComplement(b);
}

/// Tests for equality within a given tolerance, returning \c true if the
/// length of the difference vector is less than or equal to \p tolerance.
inline bool
GfIsClose(GfVec3f const &v1, GfVec3f const &v2, double tolerance)
{
    GfVec3f delta = v1 - v2;
    return delta.GetLengthSq() <= tolerance * tolerance;
}


GF_API bool
GfOrthogonalizeBasis(GfVec3f *tx, GfVec3f *ty, GfVec3f *tz,
                     bool normalize, double eps = GF_MIN_ORTHO_TOLERANCE);

GF_API void
GfBuildOrthonormalFrame(GfVec3f const &v0,
                        GfVec3f* v1,
                        GfVec3f* v2,
                        float eps = GF_MIN_VECTOR_LENGTH);

/// Returns the cross product of \p v1 and \p v2.
inline GfVec3f
GfCross(GfVec3f const &v1, GfVec3f const &v2)
{
    return GfVec3f(
        v1[1] * v2[2] - v1[2] * v2[1],
        v1[2] * v2[0] - v1[0] * v2[2],
        v1[0] * v2[1] - v1[1] * v2[0]);
}

/// Returns the cross product of \p v1 and \p v2. 
/// \see GfCross()
inline GfVec3f
operator^(GfVec3f const &v1, GfVec3f const &v2)
{
    return GfCross(v1, v2);
}

/// Spherical linear interpolation in three dimensions.
GF_API GfVec3f
GfSlerp(double alpha, GfVec3f const &v0, GfVec3f const &v1);

 
 
PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_VEC3F_H
