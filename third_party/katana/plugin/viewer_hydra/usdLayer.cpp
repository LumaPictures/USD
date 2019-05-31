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

#include "usdLayer.h"

#include <FnViewer/plugin/FnViewport.h>
#include <FnViewer/plugin/FnViewportCamera.h>
#include <FnViewer/utils/FnDrawingHelpers.h>
#include <FnViewer/utils/FnImathHelpers.h>
#include <FnViewer/plugin/FnGLStateHelper.h>

#include <OpenEXR/ImathMatrix.h>

#include <algorithm>

FnLogSetup("Viewer.USD.ViewportLayer");

using namespace IMATH_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE


USDLayer::USDLayer() : m_usdComponent(NULL) {}

USDLayer::~USDLayer() {}

void USDLayer::setup()
{
    // Get the  Hydra Viewer Delegate Component
    ViewerDelegateComponentWrapperPtr component = 
        getViewport()->getViewerDelegate()->getComponent("HydraComponent");
    if (!component)
    {
        FnLogError("HydraLayer could not find HydraComponent");
        return;
    }

    m_hydraComponent = component->getPluginInstance<HydraComponent>();
    if (!m_hydraComponent)
    {
        FnLogError("HydraLayer could not cast HydraComponent");
        return;
    }

    // Get the Katana Hydra Instance from HydraComponent
    m_hydraKatana = m_hydraComponent->getHydraKatana();
    if (!m_hydraKatana)
    {
        FnLogError("Error: HydraLayer could not get HydraKatana from HydraComponent");
        return;
    }

    // Get the USD Viewer Delegate Component
    ViewerDelegateComponentWrapperPtr usdComponent = 
        getViewport()->getViewerDelegate()->getComponent("USDComponent");
    if (!usdComponent)
    {
        FnLogError("USDLayer could not find USDComponent");
        return;
    }

    m_usdComponent = usdComponent->getPluginInstance<USDComponent>();
    if (!m_usdComponent)
    {
        FnLogError("USDLayer could not cast USDComponent");
        return;
    }
}

bool USDLayer::event(const FnEventWrapper& eventAttr)
{
    FnAttribute::GroupAttribute dataAttr = eventAttr.getData();

    // Record keyboard modifiers
    FnAttribute::IntAttribute modsAttr = dataAttr.getChildByName("modifiers");
    int modifiers = modsAttr.getValue(0, false);
    if (modifiers)
    {
        FnAttribute::IntAttribute shiftAttr =
            dataAttr.getChildByName("ShiftModifier");
        FnAttribute::IntAttribute ctrlAttr =
            dataAttr.getChildByName("ControlModifier");

        m_shiftModifier = (bool)shiftAttr.getValue(0, false);
        m_ctrlModifier = (bool)ctrlAttr.getValue(0, false);
    }
    else
    {
        m_shiftModifier = false;
        m_ctrlModifier = false;
    }

    // We let this event to go through since we only care about registering the
    // key modifiers.
    return false;
}

bool USDLayer::customPick(unsigned int x, unsigned int y,
                          unsigned int w, unsigned int h,
                          bool deepPicking,
                          PickedAttrsMap& pickedAttrs,
                          float* singlePointDepth)
{
    SdfPathSet sdfPaths;

    // Perform the picking
    HdxPickHitVector hits;
    if (m_hydraKatana->pick(getViewport(), x, y, w, h, deepPicking, hits))
    {
        // Set the Single Point Depth using the first hit, if it is requested
        if (singlePointDepth)
        {
            *singlePointDepth = hits.begin()->ndcDepth;
        }

        // Collect the rprim sdf paths that were hit, using a set to remove
        // repetitions
        for (const auto& hit : hits)
        {
            sdfPaths.insert(hit.objectId);
        }
    }

    // Get the USD-related locations that should be selected: the expanded proxy
    // locations that correspond to the picked Rprims or the whole USD locations
    // (depending on the selection mode)
    std::set<std::string> locationPaths;
    m_usdComponent->getLocationsForSelection(sdfPaths, locationPaths);

    // Update or clear the selected RPrims
    m_usdComponent->updateSelectedRPrims(sdfPaths,
                                         m_shiftModifier,
                                         m_ctrlModifier);

    // XXX:
    // If there are selected RPrims, then use a trick to force the
    // RPRIM_BOUNDS_LOCATION_PATH to be selected, so that the Camera Control
    // Layer can focus on its bounds when F is pressed.
    // See also USDComponent::updateRPrimBounds(), where the bounds are set on
    // this location and 
    if (m_usdComponent->hasSelectedRPrims())
    {
        forceBoundsLocationSelection(locationPaths);
    }

    // Location Selection mode:
    // Populate the PickedAttrsMap with the hits
    Foundry::Katana::ViewerAPI::FnPickId id = 0;
    for (const auto& locationPath : locationPaths)
    {
        pickedAttrs[id++] = FnAttribute::GroupBuilder()
            .set("location", FnAttribute::StringAttribute(locationPath))
            .build();
    }

    return true;
}

void USDLayer::forceBoundsLocationSelection(std::set<std::string>& locationPaths)
{
    if (!m_shiftModifier && !m_ctrlModifier)
    {
        // No shift nor ctrl is the only combination in which the previous
        // selection of RPRIM_BOUNDS_LOCATION_PATH will be cleared, so we
        // add it to the list of new locations to be selected
        locationPaths.insert(USDComponent::RPRIM_BOUNDS_LOCATION_PATH);
    }
    else
    {
        // In all the other cases make sure that RPRIM_BOUNDS_LOCATION_PATH
        // is part of the selection
        ViewerDelegateWrapperPtr delegate = m_usdComponent->getViewerDelegate();
        std::vector<std::string> selectedLocations;
        delegate->getSelectedLocations(selectedLocations);
        selectedLocations.push_back(USDComponent::RPRIM_BOUNDS_LOCATION_PATH);
        delegate->selectLocations(selectedLocations);
    }
}
