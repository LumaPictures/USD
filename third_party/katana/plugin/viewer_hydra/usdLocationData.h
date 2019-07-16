//
// Copyright 2018 Pixar
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
#ifndef USD_LOCATION_DATA_H_
#define USD_LOCATION_DATA_H_

#include "viewerUtils/viewerLocation.h"

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include <ImathBox.h>

PXR_NAMESPACE_USING_DIRECTIVE


/**
 * @brief USD location data to be held by usd ViewerLocations.
 *
 * These are instantiated by the USDComponent VDC and added to the location tree
 * managed by HydraComponent. The render index into which the Prims are loaded
 * are then rendered by HydraLayer ant once, together with other hydra rendered
 * location types.
 *
 * This contains the UsdImagingDelegate scene delegate responsible for
 * populating the HydraKatana's Render Index with the USD stage for the location
 * that holds this location data object. This also stores the USD stage, which
 * allows for accessing the Prims that correspond to an RPrim aded to the Render
 * Index. This also uses a UsdGeomBBoxCache to calculate/cache the bounds of
 * Rprims, used when framing the camera on selected RPrims, for example.
 */
class USDLocationData : public ViewerLocationData
{
public:
    /** @brief: Constructor. */
    USDLocationData(ViewerLocation* location,
                    UsdImagingDelegate* imagingDelegate,
                    UsdStageRefPtr stage,
                    const SdfPath& rootPath,
                    const SdfPath& referencePath);

    /** @brief: Destructor */
    virtual ~USDLocationData();

    /** @brief: Returns True if the usd related attrs changed in the location */
    bool needsReload();

    /** @brief: Updates the frame if the currentTime attr changed */
    bool updateTimeIfNeeded();

    /** @brief: Gets the USD Scene Delegate for the location.*/
    UsdImagingDelegate* getSceneDelegate();

    /** @brief: Gets the USD Stage for the location.*/
    UsdStageRefPtr getStage();

    /** @brief: Given an RPrim path, gets the corresponding Stage Prim path.*/
    SdfPath getPrimPathFromRPrimPath(const SdfPath& rprimSdfPath,
                                     bool includeReferencePath);

    /** @brief: Gets the bbox of an RPrim.
     *
     * If instanceIndex is >= 0 then we will try to resolve the instance and
     * return the local bounds of the instanced object.
     */
    bool getRPrimBounds(const SdfPath& rprimSdfPath,
                        int instanceIndex,
                        Imath::Box3d& bbox);

    /** @brief: Gets the total bbox for all the RPrims in this location.*/
    bool getTotalBounds(Imath::Box3d& bbox);

    /** @brief: Called when flushing caches. Clears the bbox cache. */
    static void flushBoundsCache();

private:
    /// The Scene Delegate that populates the Render Index with a USD stage
    UsdImagingDelegate* m_imagingDelegate;

    /// The loaded stage
    UsdStageRefPtr m_stage;

    /// The root path where the stage was placed under inside the image delegate.
    /// The Rprim paths will be defined by this root + the prim path inside the
    /// stage.
    SdfPath m_rootPath;

    /// The prim that corresponds to the reference path this is used to calculate
    /// the bounding box of Prims in relation to this reference prim.
    SdfPath m_referencePath;
    UsdPrim m_referencePrim;

    /// The root + reference path (absolute reference path in the delegate)
    SdfPath m_rootAndReferencePath;  // includes the root path (m_rootPath)

    /// Bounding box cache - avoids the re-calculation of a prim's bbox
    static UsdGeomBBoxCache* m_bboxCache;

    /// Caches the hashes of some attributes on usd locations used in triggering
    /// stage reloads and frame changes whenever these attributes change
    /// on the usd location.
    struct USDLocationAttrHashes
    {   
        FnAttribute::Hash fileName;
        FnAttribute::Hash rootLocation;
        FnAttribute::Hash referencePath;
        FnAttribute::Hash session;
        FnAttribute::Hash sessionLocation;
        FnAttribute::Hash ignoreLayerRegex;
        FnAttribute::Hash forcePopulateUsdStage;
        FnAttribute::Hash currentTime;

        USDLocationAttrHashes(FnAttribute::GroupAttribute attrs);
    };

    USDLocationAttrHashes m_hashes;

    /// Get the prim from the stage that corresponds to the given RPrim.
    /// If instanceIndex is specified, then this will resolve the instancing
    /// and return the instancing master prim. isInstanced will be set to true
    /// if this path/index pair refers to an instance, in that case the returned
    /// prim will be of the master prim being instantiated.
    UsdPrim getPrimFromRprim(const SdfPath& rprimSdfPath,
                             int instanceIndex,
                             bool* isInstanced);

    /// Gets the bounding box for a prim. If relativeToReference=true then get
    /// the bound relative to m_referencePrim. For example, instance Prims will
    /// not be relative to m_referencePrim.
    bool getPrimBounds(const UsdPrim& prim,
                       bool relativeToReference,
                       Imath::Box3d& bbox);
};

#endif
