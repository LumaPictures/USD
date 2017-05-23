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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/readPoints.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/points.h"

PXR_NAMESPACE_OPEN_SCOPE


static FnKat::Attribute
_GetPositionAttr(
        const UsdGeomPoints& points,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Because of the logic used to gather these (in PxrUsdInOp::InitUsdInArgs),
    // we can use the size of this vector as a simple test for whether motion
    // blur is enabled, without needing access to the cook interface.
    const std::vector<double>& motionSampleTimes =
        data.GetUsdInArgs()->GetMotionSampleTimes();

    if (motionSampleTimes.size() < 2)
    {
        return PxrUsdKatanaGeomGetPAttr(points, data);
    }

    const double currentTime = data.GetCurrentTime();
    const double shutterOpen = data.GetShutterOpen();
    const double shutterClose = data.GetShutterClose();

    std::vector<UsdTimeCode> sampleTimes {
        UsdTimeCode(currentTime + shutterOpen),
        UsdTimeCode(currentTime + shutterClose)
    };

    std::vector<VtVec3fArray> positionSamples(2);

    const size_t numPosSamples =
        points.ComputePositionsAtTimes(positionSamples, sampleTimes, currentTime);

    FnKat::FloatBuilder posBuilder(3);

    std::vector<float>& first = posBuilder.get(numPosSamples == 2 ? shutterOpen : 0.0f);
    PxrUsdKatanaUtils::ConvertArrayToVector(positionSamples[0], &first);
    if (numPosSamples == 2)
    {
        std::vector<float>& second = posBuilder.get(shutterClose);
        PxrUsdKatanaUtils::ConvertArrayToVector(positionSamples[1], &second);
    }

    return posBuilder.build();
}

static FnKat::Attribute
_GetWidthAttr(const UsdGeomPoints& points, double currentTime)
{
    VtFloatArray widths;
    if (!points.GetWidthsAttr().Get(&widths, currentTime))
    {
        return FnKat::Attribute();
    }

    FnKat::FloatBuilder widthsBuilder(1);
    widthsBuilder.set(std::vector<float>(widths.begin(), widths.end()));


    return widthsBuilder.build();
}

void
PxrUsdKatanaReadPoints(
        const UsdGeomPoints& points,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetCurrentTime();

    //
    // Set all general attributes for a gprim type.
    //

    PxrUsdKatanaReadGprim(points, data, attrs);

    //
    // Set more specific Katana type.
    //

    attrs.set("type", FnKat::StringAttribute("pointcloud"));

    //
    // Construct the 'geometry' attribute.
    //

    // position (with velocity-based motion samples if applicable)
    attrs.set("geometry.point.P", _GetPositionAttr(points, data));

    // velocity
    FnKat::Attribute velocitiesAttr =
        PxrUsdKatanaGeomGetVelocityAttr(points, data);
    if (velocitiesAttr.isValid())
    {
        attrs.set("geometry.point.v", velocitiesAttr);
    }

    // normals
    FnKat::Attribute normalsAttr = PxrUsdKatanaGeomGetNormalAttr(points, data);
    if (normalsAttr.isValid())
    {
        // XXX RfK doesn't support uniform curve normals.
        TfToken interp = points.GetNormalsInterpolation();
        if (interp == UsdGeomTokens->faceVarying
         || interp == UsdGeomTokens->varying
         || interp == UsdGeomTokens->vertex) {
            attrs.set("geometry.point.N", normalsAttr);
        }
    }

    // width
    FnKat::Attribute widthsAttr = _GetWidthAttr(points, currentTime);
    if (widthsAttr.isValid())
    {
        attrs.set("geometry.point.width", widthsAttr);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

