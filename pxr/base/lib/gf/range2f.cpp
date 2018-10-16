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
// range.template.cpp file to make changes.

#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/range2d.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/tf/type.h"

#include <cfloat>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfRange2f>();
}

std::ostream& 
operator<<(std::ostream &out, GfRange2f const &r)
{
    return out << '[' 
               << Gf_OstreamHelperP(r.GetMin()) << "..." 
               << Gf_OstreamHelperP(r.GetMax())
               << ']';
}

double
GfRange2f::GetDistanceSquared(const GfVec2f &p) const
{
    double dist = 0.0;

    if (p[0] < _min[0]) {
	// p is left of box
	dist += GfSqr(_min[0] - p[0]);
    }
    else if (p[0] > _max[0]) {
	// p is right of box
	dist += GfSqr(p[0] - _max[0]);
    }
    if (p[1] < _min[1]) {
	// p is front of box
	dist += GfSqr(_min[1] - p[1]);
    }
    else if (p[1] > _max[1]) {
	// p is back of box
	dist += GfSqr(p[1] - _max[1]);
    }

    return dist;
}

GfVec2f
GfRange2f::GetCorner(size_t i) const
{
    if (i > 3) {
        TF_CODING_ERROR("Invalid corner %zu > 3.", i);
        return _min;
    }

    return GfVec2f((i & 1 ? _max : _min)[0], (i & 2 ? _max : _min)[1]);
}

GfRange2f
GfRange2f::GetQuadrant(size_t i) const
{
    if (i > 3) {
        TF_CODING_ERROR("Invalid quadrant %zu > 3.", i);
        return GfRange2f();
    }

    GfVec2f a = GetCorner(i);
    GfVec2f b = .5 * (_min + _max);

    return GfRange2f(
        GfVec2f(GfMin(a[0], b[0]), GfMin(a[1], b[1])),
        GfVec2f(GfMax(a[0], b[0]), GfMax(a[1], b[1])));
}

const GfRange2f GfRange2f::UnitSquare(GfVec2f(0,0), GfVec2f(1,1));

PXR_NAMESPACE_CLOSE_SCOPE
