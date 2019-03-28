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
#include "pxr/usd/usdGeom/imagePlane.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomImagePlane,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ImagePlane")
    // to find TfType<UsdGeomImagePlane>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomImagePlane>("ImagePlane");
}

/* virtual */
UsdGeomImagePlane::~UsdGeomImagePlane()
{
}

/* static */
UsdGeomImagePlane
UsdGeomImagePlane::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomImagePlane();
    }
    return UsdGeomImagePlane(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomImagePlane
UsdGeomImagePlane::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ImagePlane");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomImagePlane();
    }
    return UsdGeomImagePlane(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdGeomImagePlane::_GetSchemaType() const {
    return UsdGeomImagePlane::schemaType;
}

/* static */
const TfType &
UsdGeomImagePlane::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomImagePlane>();
    return tfType;
}

/* static */
bool 
UsdGeomImagePlane::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomImagePlane::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomImagePlane::GetFilenameAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->infoFilename);
}

UsdAttribute
UsdGeomImagePlane::CreateFilenameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->infoFilename,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFrameAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->frame);
}

UsdAttribute
UsdGeomImagePlane::CreateFrameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->frame,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFitAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->fit);
}

UsdAttribute
UsdGeomImagePlane::CreateFitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->fit,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->offset);
}

UsdAttribute
UsdGeomImagePlane::CreateOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->offset,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetImageCenterAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->imageCenter);
}

UsdAttribute
UsdGeomImagePlane::CreateImageCenterAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->imageCenter,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetSizeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->size);
}

UsdAttribute
UsdGeomImagePlane::CreateSizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->size,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetRotateAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->rotate);
}

UsdAttribute
UsdGeomImagePlane::CreateRotateAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->rotate,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetCoverageAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->coverage);
}

UsdAttribute
UsdGeomImagePlane::CreateCoverageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->coverage,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetCoverageOriginAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->coverageOrigin);
}

UsdAttribute
UsdGeomImagePlane::CreateCoverageOriginAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->coverageOrigin,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetUseFrameExtensionAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->useFrameExtension);
}

UsdAttribute
UsdGeomImagePlane::CreateUseFrameExtensionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->useFrameExtension,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFrameOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->frameOffset);
}

UsdAttribute
UsdGeomImagePlane::CreateFrameOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->frameOffset,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFrameCacheAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->frameCache);
}

UsdAttribute
UsdGeomImagePlane::CreateFrameCacheAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->frameCache,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->width);
}

UsdAttribute
UsdGeomImagePlane::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->width,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->height);
}

UsdAttribute
UsdGeomImagePlane::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->height,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetAlphaGainAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->alphaGain);
}

UsdAttribute
UsdGeomImagePlane::CreateAlphaGainAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->alphaGain,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetDepthAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->depth);
}

UsdAttribute
UsdGeomImagePlane::CreateDepthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->depth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetSqueezeCorrectionAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->squeezeCorrection);
}

UsdAttribute
UsdGeomImagePlane::CreateSqueezeCorrectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->squeezeCorrection,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdGeomImagePlane::GetCameraRel() const
{
    return GetPrim().GetRelationship(UsdGeomTokens->camera);
}

UsdRelationship
UsdGeomImagePlane::CreateCameraRel() const
{
    return GetPrim().CreateRelationship(UsdGeomTokens->camera,
                       /* custom = */ false);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomImagePlane::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->infoFilename,
        UsdGeomTokens->frame,
        UsdGeomTokens->fit,
        UsdGeomTokens->offset,
        UsdGeomTokens->imageCenter,
        UsdGeomTokens->size,
        UsdGeomTokens->rotate,
        UsdGeomTokens->coverage,
        UsdGeomTokens->coverageOrigin,
        UsdGeomTokens->useFrameExtension,
        UsdGeomTokens->frameOffset,
        UsdGeomTokens->frameCache,
        UsdGeomTokens->width,
        UsdGeomTokens->height,
        UsdGeomTokens->alphaGain,
        UsdGeomTokens->depth,
        UsdGeomTokens->squeezeCorrection,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdGeom/camera.h"
#include <OpenImageIO/imageio.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr float inch_to_mm = 25.4f;

template <typename T>
T getAttr(const UsdAttribute& attr, const UsdTimeCode& usdTime, const T& defaultValue) {
    auto r = defaultValue;
    attr.Get(&r, usdTime);
    return r;
}

}

void
UsdGeomImagePlane::CalculateGeometryForViewport(
    VtVec3fArray* vertices,
    VtVec2fArray* uvs,
    const UsdTimeCode& usdTime) const {
    if (ARCH_UNLIKELY(vertices == nullptr)) { return; }

    UsdGeomImagePlane::ImagePlaneParams params;

    params.fileName = getAttr(GetFilenameAttr(), usdTime, SdfAssetPath(""));
    params.coverage = getAttr(GetCoverageAttr(), usdTime,  GfVec2i(-1, -1));
    params.coverageOrigin = getAttr(GetCoverageOriginAttr(), usdTime, GfVec2i(0, 0));

    SdfPathVector cameras;
    GetCameraRel().GetTargets(&cameras);

    const auto hasCamera = cameras.size() == 1;
    if (hasCamera) {
        UsdGeomCamera usdCamera(this->GetPrim().GetStage()->GetPrimAtPath(cameras[0]));
        if (!usdCamera) { return; }

        GfVec2f aperture {1.0f, 1.0f};
        usdCamera.GetHorizontalApertureAttr().Get(&aperture[0], usdTime);
        usdCamera.GetVerticalApertureAttr().Get(&aperture[1], usdTime);
        params.aperture = aperture;
        params.focalLength = getAttr(usdCamera.GetFocalLengthAttr(), usdTime, 1.0f);

        // Size is exported in inches
        // while aperture is in millimeters.
        // TODO: fix this in the maya image plane writer / translator.
        params.size = getAttr(GetSizeAttr(), usdTime, GfVec2f(-1.0f, -1.0f)) * inch_to_mm;
        params.fit = getAttr(GetFitAttr(), usdTime, UsdGeomTokens->best);
        params.rotate = getAttr(GetRotateAttr(), usdTime, 0.0f);
        params.offset = getAttr(GetOffsetAttr(), usdTime, GfVec2f(0.0f, 0.0f)) * inch_to_mm;

        params.depth = 100.0f;
        GetDepthAttr().Get(&params.depth, usdTime);
    } else {
        params.size[0] = getAttr(GetWidthAttr(), usdTime, 0.0f);
        params.size[1] = getAttr(GetHeightAttr(), usdTime, 0.0f);
        params.fit = UsdGeomTokens->toSize;
        const auto imageCenter =
            getAttr(GetImageCenterAttr(), usdTime, GfVec3f(0.0f, 0.0f, 0.0f));
        params.offset[0] = imageCenter[0];
        params.offset[1] = imageCenter[1];
        params.depth = -imageCenter[2];
        params.focalLength = 0.0f;
    }

    UsdGeomImagePlane::CalculateGeometry(vertices, uvs, params);
}


void
UsdGeomImagePlane::CalculateGeometry(
        VtVec3fArray* vertices,
        VtVec2fArray* uvs,
        UsdGeomImagePlane::ImagePlaneParams params) {
    if (ARCH_UNLIKELY(vertices == nullptr)) { return; }
    // The trick here is to take the image plane size (if not valid the camera aperture),
    // and try to fit the aperture to the image ratio, based on the fit parameter on the image plane.
    // We don't need the viewport aspect ratio / size, because it's already affecting the image by
    // affecting the projection matrix.

    if (params.size[0] <= 0.0f) {
        params.size[0] = params.aperture[0];
    }
    if (params.size[1] <= 0.0f) {
        params.size[1] = params.aperture[1];
    }

    // Doesn't matter where we divide, we'll just multiply values anyway.
    params.size[0] *= 0.5f;
    params.size[1] *= 0.5f;

    GfVec2f imageSize {100.0f, 100.0f};
    {
        auto pathString = params.fileName.GetResolvedPath();
        if (pathString.empty()){
            pathString = params.fileName.GetAssetPath();
        }
        auto* in = OIIO::ImageInput::open(pathString);
        if (in) {
            in->close();
            const auto& spec = in->spec();
            imageSize[0] = static_cast<float>(spec.width);
            imageSize[1] = static_cast<float>(spec.height);
            OIIO::ImageInput::destroy(in);
        }
    }
    const auto imageRatio = imageSize[0] / imageSize[1];
    const auto sizeRatio = params.size[0] / params.size[1];

    if (params.fit == UsdGeomTokens->fill) {
        if (imageRatio > sizeRatio) {
            params.size[0] = params.size[1] * imageRatio;
        } else {
            params.size[1] = params.size[0] / imageRatio;
        }
    } else if (params.fit == UsdGeomTokens->best) {
        if (imageRatio > sizeRatio) {
            params.size[1] = params.size[0] / imageRatio;
        } else {
            params.size[0] = params.size[1] * imageRatio;
        }
    } else if (params.fit == UsdGeomTokens->horizontal) {
        params.size[1] = params.size[0] / imageRatio;
    } else if (params.fit == UsdGeomTokens->vertical) {
        params.size[0] = params.size[1] * imageRatio;
    } else if (params.fit == UsdGeomTokens->toSize) {
    } else { assert("Invalid value passed to UsdGeomImagePlane.fit!"); }

    GfVec2f upperLeft  { -params.size[0],  params.size[1]};
    GfVec2f upperRight {  params.size[0],  params.size[1]};
    GfVec2f lowerLeft  { -params.size[0], -params.size[1]};
    GfVec2f lowerRight {  params.size[0], -params.size[1]};

    if (!GfIsClose(params.rotate, 0.0f, 0.001f)) {
        const float rsin = sinf(-params.rotate);
        const float rcos = cosf(-params.rotate);

        auto rotateCorner = [rsin, rcos] (GfVec2f& corner) {
            const float t = corner[0] * rcos - corner[1] * rsin;
            corner[1] = corner[0] * rsin + corner[1] * rcos;
            corner[0] = t;
        };

        rotateCorner(upperLeft);
        rotateCorner(upperRight);
        rotateCorner(lowerLeft);
        rotateCorner(lowerRight);
    }

    // FIXME: Offset doesn't work properly!
    upperLeft  += params.offset;
    upperRight += params.offset;
    lowerLeft  += params.offset;
    lowerRight += params.offset;
    // Both aperture and focal length should be in millimeters,
    auto projectVertex = [params] (GfVec2f& vertex) {
        vertex[0] = params.depth * vertex[0] / params.focalLength;
        vertex[1] = params.depth * vertex[1] / params.focalLength;
    };

    if (params.depth != 0.0f and params.focalLength != 0.0f){
        projectVertex(upperLeft);
        projectVertex(upperRight);
        projectVertex(lowerLeft);
        projectVertex(lowerRight);
    }

    vertices->resize(4);
    vertices->operator[](0) = GfVec3f(upperLeft[0] , upperLeft[1] , -params.depth);
    vertices->operator[](1) = GfVec3f(upperRight[0], upperRight[1], -params.depth);
    vertices->operator[](2) = GfVec3f(lowerRight[0], lowerRight[1], -params.depth);
    vertices->operator[](3) = GfVec3f(lowerLeft[0] , lowerLeft[1] , -params.depth);

    auto lerp = [] (float v, float lo, float hi) -> float {
        return lo * (1.0f - v) + hi * v;
    };

    auto clamp = [] (float v, float lo, float hi) -> float {
        if (v < lo) { return lo; }
        else if (v > hi) { return hi; }
        return v;
    };

    if (ARCH_UNLIKELY(uvs == nullptr)) { return; }
    if (params.coverage[0] <= 0.0f) {
        params.coverage[0] = imageSize[0];
    }
    if (params.coverage[1] <= 0.0f) {
        params.coverage[1] = imageSize[1];
    }
    params.coverage[0] = clamp(params.coverage[0], 0.0f, imageSize[0]);
    params.coverage[1] = clamp(params.coverage[1], 0.0f, imageSize[1]);
    params.coverageOrigin[0] = clamp(params.coverageOrigin[0], -imageSize[0], imageSize[0]);
    params.coverageOrigin[1] = clamp(params.coverageOrigin[1], -imageSize[1], imageSize[1]);

    GfVec2f minUV = {0.0f, 0.0f};
    GfVec2f maxUV = {1.0f, 1.0f};

    if (params.coverageOrigin[0] > 0) {
        minUV[0] = params.coverageOrigin[0] / imageSize[0];
        maxUV[0] = lerp(std::min(params.coverage[0], imageSize[0] - params.coverageOrigin[0]) /
                        (imageSize[0] - params.coverageOrigin[0]), minUV[0], 1.0f);
    } else if (params.coverageOrigin[0] < 0) {
        maxUV[0] = params.coverage[0] * (imageSize[0] + params.coverageOrigin[0]) / (imageSize[0] * imageSize[0]);
    } else {
        maxUV[0] = params.coverage[0] / imageSize[0];
    }

    if (params.coverageOrigin[1] > 0) {
        maxUV[1] = (imageSize[1] - params.coverageOrigin[1]) / imageSize[1];
        minUV[1] = lerp(std::min(params.coverage[1], imageSize[1] - params.coverageOrigin[1]) /
                        (imageSize[1] - params.coverageOrigin[1]), maxUV[1], 0.0f);
    } else if (params.coverageOrigin[1] < 0) {
        minUV[1] = std::min(1.0f, -params.coverageOrigin[1] / imageSize[1] +
                                  (1.0f - params.coverage[1] / imageSize[1]));
    } else {
        minUV[1] = 1.0f - params.coverage[1] / imageSize[1];
    }

    uvs->resize(4);
    uvs->operator[](0) = GfVec2f(minUV[0], minUV[1]);
    uvs->operator[](1) = GfVec2f(maxUV[0], minUV[1]);
    uvs->operator[](2) = GfVec2f(maxUV[0], maxUV[1]);
    uvs->operator[](3) = GfVec2f(minUV[0], maxUV[1]);
}

PXR_NAMESPACE_CLOSE_SCOPE
