#ifndef USDIMAGING_SPHERE_ADAPTER_H
#define USDIMAGING_SPHERE_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomImagePlane;

class UsdImagingImagePlaneAdapter : public UsdImagingGprimAdapter {
public:
    using BaseAdapter = UsdImagingGprimAdapter;

    UsdImagingImagePlaneAdapter() : UsdImagingGprimAdapter() { }

    USDIMAGING_API
    virtual ~UsdImagingImagePlaneAdapter();

    USDIMAGING_API
    virtual SdfPath Populate(
        const UsdPrim& prim,
        UsdImagingIndexProxy* index,
        const UsdImagingInstancerContext* instancerContext = nullptr) override;

    /// Thread Safe.
    USDIMAGING_API
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const*
                               instancerContext = nullptr) const override;

    USDIMAGING_API
    virtual bool IsSupported(const UsdImagingIndexProxy* index) const override;

    USDIMAGING_API
    virtual void TrackVariability(const UsdPrim& prim,
                                  const SdfPath& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  const UsdImagingInstancerContext* instancerContext) const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
