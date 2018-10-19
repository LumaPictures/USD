//
// Copyright 2017 Pixar
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

/// USDLocationData

#include "usdLocationData.h"

USDLocationData::USDLocationData(ViewerLocation* location,
                                 UsdImagingDelegate* imagingDelegate,
                                 UsdStageRefPtr stage,
                                 const SdfPath& rootPath,
                                 const SdfPath& referencePath)
    : ViewerLocationData(location),
      m_imagingDelegate(imagingDelegate),
      m_stage(stage),
      m_rootPath(rootPath),
      m_referencePath(referencePath),
      m_hashes(location->getAttrs())
{
    // Store the absolute path that includes the root and the reference paths.
    // The reference path can be empty, in that case this will just be the root
    // path, then.
    if (referencePath.IsEmpty())
    {
        m_rootAndReferencePath = m_rootPath;
    }
    else
    {
        m_rootAndReferencePath = m_rootPath.AppendPath(
            m_referencePath.MakeRelativePath(SdfPath::AbsoluteRootPath()));
    }

    // Get the reference prim that corresponds to the reference path
    m_referencePrim = m_stage->GetPrimAtPath(referencePath);
    if (!m_referencePrim.IsValid())
    {
        // Use the root prim
        m_referencePrim = m_stage->GetPseudoRoot();
    }    
}

USDLocationData::~USDLocationData()
{
    // Remove the stage's prims from the render index
    m_imagingDelegate->GetRenderIndex().RemoveSubtree(
        m_imagingDelegate->GetDelegateID(), m_imagingDelegate);

    // Delete the UsdImagingDelegate
    delete m_imagingDelegate;
}

// Static bbox cache - lazily initialized in create()
UsdGeomBBoxCache* USDLocationData::m_bboxCache = nullptr;

bool USDLocationData::needsReload()
{
    // Get the hashes for the current state of the location
    USDLocationAttrHashes hashes(getLocation()->getAttrs());

    // Do not check currentTime
    return (m_hashes.fileName                 != hashes.fileName
            || m_hashes.rootLocation          != hashes.rootLocation
            || m_hashes.referencePath         != hashes.referencePath
            || m_hashes.session               != hashes.session
            || m_hashes.sessionLocation       != hashes.sessionLocation
            || m_hashes.ignoreLayerRegex      != hashes.ignoreLayerRegex
            || m_hashes.forcePopulateUsdStage != hashes.forcePopulateUsdStage
    );
}

bool USDLocationData::updateTimeIfNeeded()
{
    FnAttribute::GroupAttribute attrs = getLocation()->getAttrs();
    FnAttribute::DoubleAttribute currentTimeAttr =
        attrs.getChildByName("currentTime");

    FnAttribute::Hash currentTimeHash = currentTimeAttr.getHash();
    if (currentTimeHash != m_hashes.currentTime)
    {
        m_imagingDelegate->SetTime( currentTimeAttr.getValue(0.0, false) );
        m_hashes.currentTime = currentTimeHash;
        return true;
    }

    return false;
}

UsdImagingDelegate* USDLocationData::getSceneDelegate()
{
    return m_imagingDelegate;
}

UsdStageRefPtr USDLocationData::getStage()
{
    return m_stage;
}

SdfPath USDLocationData::getPrimPathFromRPrimPath(
    const SdfPath& rprimSdfPath, bool includeReferencePath)
{
    if (rprimSdfPath.HasPrefix(m_rootPath))
    {
        SdfPath prefix = includeReferencePath? m_rootPath : m_rootAndReferencePath;

        // A trick to remove the m_rootPath prefix from primSdfPath
        SdfPath primPath = rprimSdfPath.MakeRelativePath(prefix);
        return primPath.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
    }

    return SdfPath(); // empty
}

bool USDLocationData::getRPrimBounds(const SdfPath& rprimSdfPath,
                                     Imath::Box3d& bbox)
{
    // Get the Prim for the given RPrim path
    UsdPrim prim = getPrimFromRprim(rprimSdfPath);
    if (!prim.IsValid()) { return false; }

    return getPrimBounds(prim, bbox);
}

bool USDLocationData::getTotalBounds(Imath::Box3d& bbox)
{
    return getPrimBounds(m_referencePrim, bbox);
}

void USDLocationData::flushBoundsCache()
{
    // Clear the bbox cache
    if (m_bboxCache)
    {
        m_bboxCache->Clear();
    }
}

UsdPrim USDLocationData::getPrimFromRprim(const SdfPath& rprimSdfPath)
{
    SdfPath primPath = getPrimPathFromRPrimPath(rprimSdfPath, true);

    if (primPath.IsEmpty())
    {
        return UsdPrim(); // invalid prim
    }

    return m_stage->GetPrimAtPath(primPath);
}

bool USDLocationData::getPrimBounds(const UsdPrim& prim,
                                    Imath::Box3d& bbox)
{
    // Lazily init the prim bbox cache
    if (!m_bboxCache)
    {
        TfTokenVector purposes;
        purposes.push_back(UsdGeomTokens->default_);
        purposes.push_back(UsdGeomTokens->proxy);
        m_bboxCache = new UsdGeomBBoxCache(UsdTimeCode::Default(), purposes);
    }

    m_bboxCache->SetTime(m_imagingDelegate->GetTime());

    // Calculate/Get the bbox for this prim from the bbox cache
    GfBBox3d bbox3d = m_bboxCache->ComputeRelativeBound(prim, m_referencePrim);
    GfRange3d range = bbox3d.ComputeAlignedRange();
    const double* rangeMin = range.GetMin().data();
    const double* rangeMax = range.GetMax().data();
    bbox.min.x = rangeMin[0]; bbox.min.y = rangeMin[1]; bbox.min.z = rangeMin[2];
    bbox.max.x = rangeMax[0]; bbox.max.y = rangeMax[1]; bbox.max.z = rangeMax[2];

    return true;
}

USDLocationData::USDLocationAttrHashes::USDLocationAttrHashes(
    FnAttribute::GroupAttribute attrs)
{
    fileName = attrs.getChildByName("fileName").getHash();
    rootLocation = attrs.getChildByName("rootLocation").getHash();
    referencePath = attrs.getChildByName("referencePath").getHash();
    session = attrs.getChildByName("session").getHash();
    sessionLocation = attrs.getChildByName("sessionLocation").getHash();
    ignoreLayerRegex = attrs.getChildByName("ignoreLayerRegex").getHash();
    forcePopulateUsdStage = attrs.getChildByName("forcePopulateUsdStage").getHash();
    currentTime = attrs.getChildByName("currentTime").getHash();
}
