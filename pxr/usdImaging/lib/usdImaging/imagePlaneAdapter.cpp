#include "pxr/usdImaging/usdImaging/imagePlaneAdapter.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"

#include "pxr/usd/usdGeom/imagePlane.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USD_IMAGING_ENABLE_IMAGEPLANES, true,
                      "Enables/disables the use of image planes in hydra until the code matures enough.");

namespace {

// Simplest right handed vertex counts and vertex indices.
const VtIntArray _faceVertexCounts = {
    3, 3
};

const VtIntArray _faceVertexIndices = {
    0, 1, 2, 0, 2, 3
};

const VtIntArray _holeIndices;

bool
_isImagePlaneEnabled() {
    static const auto _enabled = TfGetEnvSetting(USD_IMAGING_ENABLE_IMAGEPLANES);
    return _enabled;
}

}

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingImagePlaneAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingImagePlaneAdapter::~UsdImagingImagePlaneAdapter() {

}

SdfPath
UsdImagingImagePlaneAdapter::Populate(
    const UsdPrim& prim,
    UsdImagingIndexProxy* index,
    const UsdImagingInstancerContext* instancerContext /*= nullptr*/){
    return _isImagePlaneEnabled() ? _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext) : SdfPath();
}

void
UsdImagingImagePlaneAdapter::TrackVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const {
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
    // We can check here if coverage or coverage origin is animated and
    // turn off varying for primvars.
    *timeVaryingBits |= HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyExtent;
}

void
UsdImagingImagePlaneAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const*
    instancerContext /*= nullptr*/) const {
    BaseAdapter::UpdateForTime(prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // Update the transform with the size authored for the sphere.
        GfMatrix4d& ctm = valueCache->GetTransform(cachePath);
        GfMatrix4d xf; xf.SetIdentity();
        ctm = xf * ctm;
    }

    if (requestedBits & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyExtent)) {
        UsdGeomImagePlane imagePlane(prim);
        VtVec3fArray vertices;
        VtVec2fArray uvs;
        imagePlane.CalculateGeometryForViewport(
            &vertices, &uvs, time);

        if (requestedBits & HdChangeTracker::DirtyPoints) {
            valueCache->GetPoints(cachePath) = vertices;

            _MergePrimvar(
                &valueCache->GetPrimvars(cachePath),
                HdTokens->points,
                HdInterpolationVertex,
                HdPrimvarRoleTokens->point);
        }

        if (requestedBits & HdChangeTracker::DirtyPrimvar) {
            valueCache->GetPrimvar(cachePath, TfToken("st")) = uvs;

            _MergePrimvar(
                &valueCache->GetPrimvars(cachePath),
                TfToken("st"),
                HdInterpolationVertex);
        }

        if (requestedBits & HdChangeTracker::DirtyExtent) {
            // This does not change the extent representation in the viewport,
            // but affects the frustrum culling.
            // This also freaks out the min / max depth calculation.
            GfRange3d extent;
            for (const auto& vertex: vertices) {
                extent.ExtendBy(vertex);
            }
            valueCache->GetExtent(cachePath) = extent;
        }
    }

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);

        topology = HdMeshTopology(
            UsdGeomTokens->triangleSubdivisionRule,
            UsdGeomTokens->rightHanded,
            _faceVertexCounts,
            _faceVertexIndices,
            _holeIndices, 0);
    }
}

bool
UsdImagingImagePlaneAdapter::IsSupported(const UsdImagingIndexProxy* index) const {
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

PXR_NAMESPACE_CLOSE_SCOPE
