#include "pxrUsdTranslators/imagePlaneWriter.h"

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/imagePlane.h>
#include <pxr/base/gf/range3f.h>
#include <maya/MBoundingBox.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MRenderUtil.h>

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/writeJobContext.h"
#include "usdMaya/writeUtil.h"

#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/imaging/hio/glslfx.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MRenderUtil.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(imagePlane, MayaImagePlaneWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(imagePlane, UsdGeomImagePlane);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((defaultOutputName, "out"))
    ((materialName, "HdMaterial"))
    ((shaderName, "HdShader"))
    ((primvarName, "HdPrimvar"))
    ((textureName, "HdTexture"))
    (st)
    (result)
    (colorOpacity)
    (color)
    (rgba)
    (file)
    (varname)
);

MayaImagePlaneWriter::MayaImagePlaneWriter(
    const MFnDependencyNode& depNodeFn, const SdfPath& uPath, UsdMayaWriteJobContext& jobCtx)
    : UsdMayaPrimWriter(depNodeFn, uPath, jobCtx)
{
    auto primSchema =
        UsdGeomImagePlane::Define(GetUsdStage(), GetUsdPath());
    TF_AXIOM(primSchema);
    _usdPrim = primSchema.GetPrim();
    TF_AXIOM(_usdPrim);

    const auto materialPath = GetUsdPath().AppendChild(_tokens->materialName);
    auto material = UsdShadeMaterial::Define(GetUsdStage(), materialPath);
    auto shader = UsdShadeShader::Define(GetUsdStage(),
        materialPath.AppendChild(_tokens->shaderName));
    auto primvar = UsdShadeShader::Define(GetUsdStage(),
        materialPath.AppendChild(_tokens->primvarName));
    auto texture = UsdShadeShader::Define(GetUsdStage(),
        materialPath.AppendChild(_tokens->textureName));
    mTexture = texture.GetPrim();
    TF_AXIOM(mTexture);

    UsdShadeMaterialBindingAPI(_usdPrim)
        .Bind(material);

    UsdShadeConnectableAPI::ConnectToSource(
        UsdShadeMaterial(material).CreateSurfaceOutput(HioGlslfxTokens->glslfx),
        UsdShadeMaterial(shader).CreateSurfaceOutput());

    shader.CreateIdAttr().Set(UsdImagingTokens->UsdImagePlaneSurface);

    primvar.CreateIdAttr().Set(UsdImagingTokens->UsdPrimvarReader_float2);

    texture.CreateIdAttr().Set(UsdImagingTokens->UsdUVTexture);
    texture.GetPrim()
        .CreateAttribute(UsdHydraTokens->textureMemory,
            SdfValueTypeNames->Float, SdfVariabilityUniform)
        .Set(10.0f * 1024.0f * 1024.0f);

    UsdShadeConnectableAPI shaderApi(shader);
    UsdShadeConnectableAPI primvarApi(primvar);
    UsdShadeConnectableAPI textureApi(texture);

    primvarApi.CreateInput(_tokens->varname, SdfValueTypeNames->Token)
        .Set(_tokens->st);

    UsdShadeConnectableAPI::ConnectToSource(
        textureApi.CreateInput(_tokens->st, SdfValueTypeNames->Float2),
        primvarApi.CreateOutput(_tokens->result, SdfValueTypeNames->Float2));

    UsdShadeConnectableAPI::ConnectToSource(
        shaderApi.CreateInput(_tokens->colorOpacity, SdfValueTypeNames->Float4),
        textureApi.CreateOutput(_tokens->rgba, SdfValueTypeNames->Float4));

    for (auto pt = GetUsdPath(); !pt.IsEmpty(); pt = pt.GetParentPath()) {
        auto pr = GetUsdStage()->GetPrimAtPath(pt);
        if (pr && pr.IsA<UsdGeomCamera>()) {
            primSchema.CreateCameraRel().AddTarget(pt);
            break;
        }
    }
}

MayaImagePlaneWriter::~MayaImagePlaneWriter() {
}

void MayaImagePlaneWriter::Write(const UsdTimeCode& usdTime) {
    UsdGeomImagePlane primSchema(_usdPrim);

    // Write the attrs
    _WriteImagePlaneAttrs(usdTime, primSchema);
}

bool MayaImagePlaneWriter::_WriteImagePlaneAttrs(
    const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema) {
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return true;
    }

    // Write extent, just the default for now. It should be setup in the adapter for drawing.
    MFnDagNode dnode(GetDagPath());
    
    auto imageNameExtracted =
        MRenderUtil::exactImagePlaneFileName(dnode.object());
    const SdfAssetPath imageNameExtractedPath(
        std::string(imageNameExtracted.asChar()));
    const auto sizePlug = dnode.findPlug("size");
    const auto imageName = SdfAssetPath(
        std::string(dnode.findPlug("imageName").asString().asChar()));
    primSchema.GetFilenameAttr().Set(imageName);
    primSchema.GetFilenameAttr().Set(imageNameExtractedPath, usdTime);
    UsdShadeShader textureShader(mTexture);
    auto filenameAttr = UsdShadeConnectableAPI(textureShader)
        .CreateInput(_tokens->file, SdfValueTypeNames->Asset);
    filenameAttr.Set(imageNameExtractedPath, usdTime);
    filenameAttr.Set(imageName);
    const auto fit = dnode.findPlug("fit").asShort();
    if (fit == UsdGeomImagePlane::FIT_BEST) {
        primSchema.GetFitAttr().Set(UsdGeomTokens->best);
    } else if (fit == UsdGeomImagePlane::FIT_FILL) {
        primSchema.GetFitAttr().Set(UsdGeomTokens->fill);
    } else if (fit == UsdGeomImagePlane::FIT_HORIZONTAL) {
        primSchema.GetFitAttr().Set(UsdGeomTokens->horizontal);
    } else if (fit == UsdGeomImagePlane::FIT_VERTICAL) {
        primSchema.GetFitAttr().Set(UsdGeomTokens->vertical);
    } else if (fit == UsdGeomImagePlane::FIT_TO_SIZE) {
        primSchema.GetFitAttr().Set(UsdGeomTokens->toSize);
    }
    primSchema.GetUseFrameExtensionAttr().Set(
        dnode.findPlug("useFrameExtension").asBool());
    _SetAttribute(primSchema.GetFrameOffsetAttr(),
        dnode.findPlug("frameOffset").asInt(), usdTime);
    _SetAttribute(primSchema.GetWidthAttr(),
        dnode.findPlug("width").asFloat(), usdTime);
    _SetAttribute(primSchema.GetHeightAttr(),
        dnode.findPlug("height").asFloat(), usdTime);
    _SetAttribute(primSchema.GetAlphaGainAttr(),
        dnode.findPlug("alphaGain").asFloat(), usdTime);
    _SetAttribute(primSchema.GetDepthAttr(),
        dnode.findPlug("depth").asFloat(), usdTime);
    _SetAttribute(primSchema.GetSqueezeCorrectionAttr(),
        dnode.findPlug("squeezeCorrection").asFloat(), usdTime);
    const auto offsetPlug = dnode.findPlug("offset");
    _SetAttribute(primSchema.GetOffsetAttr(),
                  GfVec2f(offsetPlug.child(0).asFloat(),
                      offsetPlug.child(1).asFloat()), usdTime);
    _SetAttribute(primSchema.GetSizeAttr(),
                  GfVec2f(sizePlug.child(0).asFloat(),
                      sizePlug.child(1).asFloat()), usdTime);
    _SetAttribute(primSchema.GetRotateAttr(),
        dnode.findPlug("rotate").asFloat(), usdTime);
    const auto coveragePlug = dnode.findPlug("coverage");
    _SetAttribute(primSchema.GetCoverageAttr(),
                  GfVec2i(coveragePlug.child(0).asInt(),
                      coveragePlug.child(1).asInt()), usdTime);
    const auto coverageOriginPlug = dnode.findPlug("coverageOrigin");
    _SetAttribute(primSchema.GetCoverageOriginAttr(),
                  GfVec2i(coverageOriginPlug.child(0).asInt(),
                      coverageOriginPlug.child(1).asInt()), usdTime);
    const auto imageCenterPlug = dnode.findPlug("imageCenter");
    _SetAttribute(primSchema.GetImageCenterAttr(),
                  GfVec3f(imageCenterPlug.child(0).asFloat(),
                          imageCenterPlug.child(1).asFloat(),
                          imageCenterPlug.child(2).asFloat()));
    VtVec3fArray positions;
    primSchema.CalculateGeometryForViewport(&positions, nullptr, usdTime);
    GfRange3f extent;
    for (const auto& vertex: positions) {
        extent.ExtendBy(vertex);
    }
    VtArray<GfVec3f> extents(2);
    extents[0] = extent.GetMin();
    extents[1] = extent.GetMax();
    _SetAttribute(primSchema.CreateExtentAttr(), extents, usdTime);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
