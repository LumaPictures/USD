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

#include "usdComponent.h"

#include <FnViewer/utils/FnImathHelpers.h>

#include "usdKatana/cache.h"
#include "usdKatana/locks.h"

#include <ImathBoxAlgo.h>

#include <iostream>
#include <string>
#include <ctime>
#include <algorithm>

using namespace Foundry::Katana::ViewerUtils;

FnLogSetup("Viewer.USD.ViewerDelegateComponent");


USDComponent::USDComponent()
    : m_hydraComponent(nullptr),
      m_tree(nullptr),
      m_useRprimSelection(false)
{}

USDComponent::~USDComponent()
{}

const std::string USDComponent::SDF_PATH_ROOT = "/USD";

const std::string USDComponent::RPRIM_BOUNDS_LOCATION_PATH = "/root/usd_bounds";

Foundry::Katana::ViewerAPI::ViewerDelegateComponent* USDComponent::create()
{
    return new USDComponent();
}

void USDComponent::flush()
{
    // flush all caches
    UsdKatanaCache::GetInstance().Flush();
    USDLocationData::flushBoundsCache();
}

bool USDComponent::initHydraComponentRef()
{
    // Get the ViewerDelegateComponent
    ViewerDelegateComponentWrapperPtr component = 
        getViewerDelegate()->getComponent("HydraComponent");
    if (!component)
    {
        FnLogError("USDComponent could not find HydraComponent");
        return false;
    }

    m_hydraComponent = component->getPluginInstance<HydraComponent>();
    if (!m_hydraComponent)
    {
        FnLogError("HydraLayer could not cast HydraComponent");
        return false;
    }

    m_hydraKatana = m_hydraComponent->getHydraKatana();
    m_tree = m_hydraComponent->getTree();

    // Init the selection color
    updateSelectionColor();

    return true;
}


bool USDComponent::locationEvent(const ViewerLocationEvent& event,
                                 bool locationHandled)
{
    // Initialize the HydraComponent reference if not done yet
    if (!m_hydraComponent && !initHydraComponentRef()) { return false; }

    // Location Added / Updated
    if (event.stateChanges.attributesUpdated)
    {
        // Get the Location cache
        ViewerLocation* location = m_tree->get(event.locationPath);

        if (location)
        {
            bool dirty = false;

            FnAttribute::GroupAttribute attrs = event.attributes;

            FnAttribute::StringAttribute typeAttr = attrs.getChildByName("type");
            if (typeAttr.getValue("", false) == "usd")
            {
                USDLocationData* data =
                    dynamic_cast<USDLocationData*>(location->getData());

                // Check if there is already usd data on the location and if
                // so, if the stage needs to be reloaded into Hydra. If there is
                // no data, then the stage needs to be loaded for the first
                // time.
                if (!data || data->needsReload())
                {
                    // Load the USD stage into the Hydra Render Index
                    loadUsd(location);
                    dirty = true;
                }
                else if (data)
                {
                    // Update the frame if it has changed since the last time
                    dirty = data->updateTimeIfNeeded();
                }
            }

            if (event.stateChanges.localXformUpdated)
            {
                // Update the xform in the USD descendants
                propagateXform(location);
                dirty = true;
            }

            if (dirty)
            {
                setViewportsDirty();
            }
        }
    }

    return false;
}

void USDComponent::locationsSelected(
    const std::vector<std::string>& locationPaths)
{
    // Go get the HydraComponent if not done yet
    if (!m_hydraComponent && !initHydraComponentRef()) { return; }

    // Construct a vector of SdfPaths of the given locations
    SdfPathSet sdfPaths;

    if (!m_useRprimSelection)
    {
        //TODO: do not iterate superfluous location paths
        for (const auto& locationPath : locationPaths)
        {
            // Check if the location is in the tree cache and has USD data then
            // highlight all the rprims below it in Hydra
            ViewerLocation* location = m_tree->get(locationPath);

            if (location)
            {
                // Get all the usd locations below the selected location
                std::vector<ViewerLocation*> usdDescendants;
                getAllUsdDescendants(location, &usdDescendants);

                // Higlight every prim below the usd locations
                for (auto usdLocation : usdDescendants)
                {
                    // Get the sdf path
                    SdfPath rootUsdPath = locationPathToRPrimSdfPath(
                        usdLocation->getPath());

                    // Add the root location to the rprims list
                    sdfPaths.insert(rootUsdPath);
                }
            }
        }
    }

    // Append the selected RPrims
    sdfPaths.insert(m_selectedRPrims.begin(), m_selectedRPrims.end());

    highlightRprims(sdfPaths, false);
}

FnAttribute::DoubleAttribute USDComponent::getBounds(
    const std::string& locationPath)
{
    if (locationPath == RPRIM_BOUNDS_LOCATION_PATH)
    {
        return m_selectedRPrimsBounds;
    }
    else
    {
        // Check if this is indeed a usd location with valid data
        ViewerLocation* usdLocation = m_tree->get(locationPath);
        if (usdLocation)
        {
            // Get the usd location data 
            USDLocationData* usdData = dynamic_cast<USDLocationData*>(
                usdLocation->getData());
            if (usdData)
            {
                // Get the Rprim at the root of the stage
                SdfPath rprimSdfPath = locationPathToRPrimSdfPath(locationPath);

                // Ask USDLocationData for the bounds of the root rprim 
                Imath::Box3d bbox;
                if (usdData->getTotalBounds(bbox))
                {
                    // Convert Imath::Box3d to DoubleAttribute
                    return convertBoundsToAttr(bbox);
                }
            }
        }
    }

    return FnAttribute::Attribute(); // invalid attr
}

FnAttribute::Attribute USDComponent::getOption(
    OptionIdGenerator::value_type optionId)
{
    static const auto selectRprimsOptionId =
        OptionIdGenerator::GenerateId("SelectRprims");

    if (selectRprimsOptionId == optionId)
    {
        return FnAttribute::IntAttribute(m_useRprimSelection? 1 : 0);
    }

    return ViewerDelegateComponent::getOption(optionId);
}

void USDComponent::setOption(OptionIdGenerator::value_type optionId,
                             FnAttribute::Attribute attr)
{
    static const auto selectRprimsOptionId =
        OptionIdGenerator::GenerateId("SelectRprims");

    if (selectRprimsOptionId == optionId)
    {
        // Update the selection mode or leave it as is if attr is invalid
        FnAttribute::IntAttribute intAttr(attr);

        if (intAttr.isValid())
        {
            bool value = (bool)intAttr.getValue(m_useRprimSelection, false);

            if (value != m_useRprimSelection)
            {
                m_useRprimSelection = value;


            // Clear the rprim selection, bounds and highlighting
            m_selectedRPrims.clear();
            updateSelectedRPrimBounds();
            highlightRprims(m_selectedRPrims, true);

            }
        }

        // Change the selection color according to the selection mode
        updateSelectionColor();

        return;
    }

    ViewerDelegateComponent::setOption(optionId, attr);
}

bool USDComponent::hasSelectedRPrims()
{
    return !m_selectedRPrims.empty();
}

void USDComponent::getLocationsForSelection(const SdfPathSet& rprimSdfPaths,
                                            std::set<std::string>& locationPaths)
{
    if (m_useRprimSelection)
    {
        for (auto sdfPath : rprimSdfPaths)
        {
            // In RPrim selection we are only interested in expanded locations
            // that correspond to RPrims that are under proxies, since RPrims on
            // non-proxy usd locations do not exist as locations themselves.

            ViewerLocation* usdLocation = findNearestUSDAncestor(sdfPath);
            if (!usdLocation) { continue; }

            // If this location is not a proxy location, then no expanded
            // locations should be selected
            if (!usdLocation->isVirtual()) { continue; }

            // Check if the usd location has data 
            USDLocationData* usdData = dynamic_cast<USDLocationData*>(
                usdLocation->getData());
            if (!usdData) { continue; }

            // Get the first non-proxy location on or above the usd location
            ViewerLocation* nonProxyLocation = m_tree->findNearestAncestor(
                usdLocation->getPath(), true /*ignoreVirtualLocations*/);
            if (!nonProxyLocation) { continue; }

            // Get the Prim path, which will be appended to the usd location
            // once expanded
            SdfPath primSdfPath = usdData->getPrimPathFromRPrimPath(sdfPath,
                                                false/*includeReferencePath*/);

            // Construct the expanded path by appending the Prim path to the
            // usd Path
            std::string locationPath = nonProxyLocation->getPath() +
                                       primSdfPath.GetString();
            locationPaths.insert(locationPath);
        }
    }
    else
    {
        for (auto sdfPath : rprimSdfPaths)
        {            
            // Select USD locations for the given RPrim paths:
            ViewerLocation* location = getUSDLocationOfRPrimPath(sdfPath);
            if (location)
            {
                locationPaths.insert(location->getPath());
            }
        }
    }
}

void USDComponent::updateSelectedRPrims(const SdfPathSet& rprimSdfPaths,
                                        bool shift, bool ctrl)
{
    if (m_useRprimSelection)
    {
        // Use the shift and ctrl modifiers to add/remove selected RPrims
        if (!shift && !ctrl)
        {
            // no Shift nor Ctrl: Substitute
            m_selectedRPrims = rprimSdfPaths;
        }
        else if (shift && ctrl)
        {
            // Shift+Ctrl: Add all
            m_selectedRPrims.insert(rprimSdfPaths.begin(), rprimSdfPaths.end());
        }
        else if (ctrl)
        {
            // Ctrl : Remove selected
            for (auto sdfPath : rprimSdfPaths)
            {
                m_selectedRPrims.erase(sdfPath);
            }
        }
        else if (shift)
        {
            // Shift : Flip
            for (auto sdfPath : rprimSdfPaths)
            {
                auto it = m_selectedRPrims.find(sdfPath);
                if (it != m_selectedRPrims.end())
                {
                    // Shift on selected locations : Remove
                    m_selectedRPrims.erase(it);
                }
                else
                {
                    // Shift no not selected locations: Add
                    m_selectedRPrims.insert(sdfPath);
                }
            }
        }
    }
    else
    {
        m_selectedRPrims.clear();
    }

    // Update the bounding box on the ficticious location used when framing the
    // camera on the selected rprims
    updateSelectedRPrimBounds();

    // highlight the selected rprims
    highlightRprims(m_selectedRPrims, true);
}

void USDComponent::setViewportsDirty()
{
    // Mark all viewports as dirty
    unsigned int n = getViewerDelegate()->getNumberOfViewports();
    for (unsigned int i = 0; i < n; ++i)
    {
        getViewerDelegate()->getViewport(i)->setDirty(true);
    }
}

bool USDComponent::loadUsd(ViewerLocation* location)
{ 
    FnAttribute::GroupAttribute attrs = location->getAttrs();
    std::string locationPath = location->getPath();

    // Get usd file path
    FnKat::StringAttribute usdFileAttr = attrs.getChildByName("fileName");
    std::string usdFile = usdFileAttr.getValue("", false);
    if (usdFile.empty())
    {
        return false;
    }

    // The reference path scopes what portion of the stage is loaded.
    FnKat::StringAttribute usdReferencePathAttr = attrs.getChildByName("referencePath");
    SdfPath usdReferencePath;
    if (usdReferencePathAttr.isValid())
    {
        usdReferencePath = SdfPath(usdReferencePathAttr.getValue());
    }

    // Session attrs
    FnKat::GroupAttribute sessionAttr = attrs.getChildByName("session");
    FnKat::StringAttribute sessionLocationAttr = attrs.getChildByName("sessionLocation");
    if (!sessionLocationAttr.isValid())
    {
        sessionLocationAttr = attrs.getChildByName("rootLocation");
    }
    std::string sessionLocation = sessionLocationAttr.getValue("", false);
   
    // Ignore layer attr
    FnKat::StringAttribute ignoreLayerAttr= attrs.getChildByName("ignoreLayerRegex");
    std::string ignoreLayerRegex = ignoreLayerAttr.getValue("$^", false);

    // Force populate
    FnKat::FloatAttribute forcePopulateAttr =  attrs.getChildByName("forcePopulateUsdStage");
    bool forcePopulate = forcePopulateAttr.getValue((float)true,false);

    // Current time attr
    FnKat::DoubleAttribute currentTimeAttr = attrs.getChildByName("currentTime");

    // The multi-threaded Usd Op may be loading or unloading models on the stage
    // we need, so we grab the global lock in reader mode.
    boost::shared_lock<boost::upgrade_mutex> readerLock(UsdKatanaGetStageLock());

    // Get Stage
    auto stage = UsdKatanaCache::GetInstance().GetStage( usdFile, 
                                                         sessionAttr,
                                                         sessionLocation,
                                                         ignoreLayerRegex,
                                                         forcePopulate);
    if (!stage)
    {
        FnLogWarn(std::string("Cannot resolve path: ") + usdFile.c_str());
        return false;
    }

    // Get root prim
    UsdPrim prim;
    if (usdReferencePath.IsEmpty())
    {
        prim = stage->GetPseudoRoot();
    }
    else
    {
        prim = stage->GetPrimAtPath(usdReferencePath);
    }

    if (!prim)
    {
        FnLogWarn(std::string("Cannot compose ") + prim.GetPath().GetString());
        return false;
    }

    // Delete any existing data on the location
    ViewerLocationData* data = location->getData();
    if (data)
    {
        if (dynamic_cast<USDLocationData*>(data))
        {
            delete data;
        }
        else
        {
            FnLogWarn(std::string("Usd Location has non USD data: ") + locationPath);
            return false;
        }        
    }

    // Get the root SdfPath under which the stage will be placed
    SdfPath rootPath = locationPathToRPrimSdfPath(locationPath);

    // Create the scene delegate
    UsdImagingDelegate* usdImagingDelegate = new UsdImagingDelegate(
        m_hydraKatana->getRenderIndex(), rootPath);
    
     // Populate the Render Index
    usdImagingDelegate->Populate(prim);

    // Set the frame
    usdImagingDelegate->SetTime( currentTimeAttr.getValue(0, false) );

    // Set the location xform
    GfMatrix4d xform = toGfMatrixd(location->getWorldXform().getValue());
    usdImagingDelegate->SetRootTransform(xform);

    if (!m_useRprimSelection)
    {
        // Check if the usd content needs to be highllighted by checking if any
        // parent location is selected. This only makes sense in USD location
        // selection mode, not in RPrim selection.
        if (location->isSelected() || location->isAncestorSelected())
        {
            SdfPathSet selection;
            selection.insert(rootPath);
            highlightRprims(selection, false);
        }
    }

    // Set the new Data
    location->setData(
        new USDLocationData(location,
                            usdImagingDelegate,
                            stage,
                            rootPath,
                            usdReferencePath));

    return true;
}

void USDComponent::propagateXform(ViewerLocation* location)
{
    // Update children
    std::vector<ViewerLocation*> children;
    location->getChildren(children);
    for (auto child : children)
    {
        // Don't update the child's xform if it is absolute
        if (!child->isLocalXformAbsolute())
        {
            // If this location has data associated, then it is a usd location
            USDLocationData* data = dynamic_cast<USDLocationData*>(child->getData());
            if (data)
            {
                // Update the xform
                GfMatrix4d xform = toGfMatrixd(child->getWorldXform().getValue());
                data->getSceneDelegate()->SetRootTransform(xform);
            }

            propagateXform(child);
        }
    }
}

void USDComponent::highlightRprims(const SdfPathSet& sdfPaths, bool replace)
{
    // Remove all irrelevant descendants
    SdfPathVector sdfPathsVec;
    sdfPathsVec.assign(sdfPaths.begin(), sdfPaths.end());
    SdfPath::RemoveDescendentPaths(&sdfPathsVec);

    // Get all descendants
    SdfPathSet sdfPathsToHighlight;
    for (auto& sdfPath : sdfPathsVec)
    {
        sdfPathsToHighlight.insert(sdfPath);

        SdfPathVector const& rprimPaths =
            m_hydraKatana->getRenderIndex()->GetRprimSubtree(sdfPath);

        for (const auto& rprimPath : rprimPaths)
        {
            sdfPathsToHighlight.insert(rprimPath);
        }
    }

    // Set selection in Hydra
    m_hydraKatana->select(sdfPathsToHighlight, replace);

    setViewportsDirty();
}

void USDComponent::updateSelectionColor()
{
    if (m_useRprimSelection)
    {
        // RPrim mode
        m_hydraKatana->setSelectionColor(1, 0.8, 0.1, 0.6);
    }
    else
    {
        // USD location mode
        m_hydraKatana->setSelectionColor(0.1, 1, 1, 0.6);
    }
}

SdfPath USDComponent::locationPathToRPrimSdfPath(const std::string& locationPath)
{
    // Create a root SdfPath for this usd
    return SdfPath(SDF_PATH_ROOT + locationPath);
}

std::string USDComponent::rprimSdfPathToLocationPath(const SdfPath& rprimSdfPath)
{
    auto path = rprimSdfPath.GetString();

    // Check if the sdf path starts with the expected root path for all the usd
    // locations
    if (path.compare(0, SDF_PATH_ROOT.size(), SDF_PATH_ROOT) != 0)
    {
        return ""; 
    }

    // Make sure the path has no '.'
    std::replace(path.begin(), path.end(), '.', '_');

    // Get the path without the root
    return path.substr(SDF_PATH_ROOT.size());
}

ViewerLocation* USDComponent::getUSDLocationOfRPrimPath(const SdfPath& rprimSdfPath)
{
    // Get the full location without the /USD prefix, this will be an
    // empty string if it is not a /USD location
    std::string locationPath = rprimSdfPathToLocationPath(rprimSdfPath);

    if (locationPath.empty())
    {
        return nullptr;
    }

    // Find the nearest ancestor of this location path that has been
    // cooked and consequently is present in the tree location cache.
    ViewerLocation* location = findNearestUSDAncestor(locationPath);

    // Get the nearest non-virtual ancestor above, if the usd locaiton is virtual
    if (location && location->isVirtual())
    {
        location = m_tree->findNearestAncestor(location->getPath(),
                                               true /*ignoreVirtualLocations*/);
    }

    return location;
}

void USDComponent::getAllUsdDescendants(ViewerLocation* location,
    std::vector<ViewerLocation*>* usdLocations)
{
    // If this location has usd data associated, then it is a usd location
    if (dynamic_cast<USDLocationData*>(location->getData()))
    {
        usdLocations->push_back(location);
    }

    std::vector<ViewerLocation*> children;
    location->getChildren(children);

    for (auto child : children)
    {
        getAllUsdDescendants(child, usdLocations);
    }
}

ViewerLocation* USDComponent::findNearestUSDAncestor(
    const std::string& locationPath)
{
    // Find the nearest ancestor of this location path that has been
    // cooked and consequently is present in the tree location cache.
    ViewerLocation* location = m_tree->findNearestAncestor(locationPath);

    // Continue iterating up the ancestors until we find a location with
    // USD data (i.e. a successfully loaded usd location).
    while(location)
    {
        // Check if the location contains custom data and if it is USD
        // type of data.
        if (dynamic_cast<USDLocationData*>(location->getData()))
        {
            return location;
        }

        // Iterate up
        location = location->getParent();
    }

    return location;
}

ViewerLocation* USDComponent::findNearestUSDAncestor(const SdfPath& rprimSdfPath)
{
    // Extract a location path from the SdfPath, this will contain the stage's
    // root location appended with the prim path
    std::string locationPath = rprimSdfPathToLocationPath(rprimSdfPath);
    if (locationPath.empty()) { return nullptr; }

    // Get the nearest cached location above the resulting path
    return findNearestUSDAncestor(locationPath);
}


FnAttribute::DoubleAttribute USDComponent::convertBoundsToAttr(
        const Imath::Box3d& bbox)
{
    double bboxValues[6] = { bbox.min.x, bbox.max.x,
                             bbox.min.y, bbox.max.y,
                             bbox.min.z, bbox.max.z };

    return FnAttribute::DoubleAttribute(bboxValues, 6, 3);
}

void USDComponent::updateSelectedRPrimBounds()
{
    FnAttribute::Attribute bboxAttr; // default to invalid attr

    if ( !m_selectedRPrims.empty() )
    {
        Imath::Box3d totalBbox;

        bool foundBounds = false;
        for (auto rprimSdfPath : m_selectedRPrims)
        {
            // Check if this is a rprim from an existing usd location
            ViewerLocation* usdLocation = findNearestUSDAncestor(rprimSdfPath);
            if (!usdLocation) { continue; }

            // Check if the usd location has data 
            USDLocationData* usdData = static_cast<USDLocationData*>(
                usdLocation->getData());
            if (!usdData) { continue; }

            // Get the bbox and extend the total bbox with it
            Imath::Box3d bbox;
            if (usdData->getRPrimBounds(rprimSdfPath, bbox))
            {   
                // Convert the bounds to world space and extend the total bbox
                bbox = Imath::transform(bbox, usdLocation->getWorldXform());
                totalBbox.extendBy(bbox);
                foundBounds = true;
            }
        }

        if (foundBounds)
        {
            bboxAttr = convertBoundsToAttr(totalBbox);
        }
        
    }

    // Update the cahced bounds attr
    m_selectedRPrimsBounds = bboxAttr;
}
