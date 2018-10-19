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

#include "hydraComponent.h"

#include <FnViewer/utils/FnImathHelpers.h>

#include "usdKatana/cache.h"
#include "usdKatana/locks.h"

#include <iostream>
#include <string>
#include <ctime>

using namespace Foundry::Katana::ViewerUtils;

FnLogSetup("Viewer.Hydra.ViewerDelegateComponent");

/// HydraComponent

HydraComponent::HydraComponent()
{
    // Get the HydraKatana from the cache
    m_hydraKatana = HydraKatana::Create();
}

HydraComponent::~HydraComponent() {}

Foundry::Katana::ViewerAPI::ViewerDelegateComponent* HydraComponent::create()
{
    return new HydraComponent();
}

void HydraComponent::flush() {}

bool HydraComponent::locationEvent(const ViewerLocationEvent& event, bool locationHandled)
{
    bool redraw = false;

    // Location Added / Updated
    if (event.stateChanges.attributesUpdated)
    {
        FnAttribute::GroupAttribute attrs = event.attributes;

        // Get the local xform 
        Imath::M44d localXform = toImathMatrix44d(event.localXformMatrix);

        // Update the tree cache
        m_tree.addOrUpdate(event.locationPath, attrs, event.isVirtualLocation,
            localXform, event.localXformIsAbsolute);

        redraw = true;
    }

    // Location Removed
    if (event.stateChanges.locationRemoved)
    {
        // Remove the prims from the render index and delete all the
        // HdSceneDelegates in this locations and its descendants
        m_tree.remove(event.locationPath);

        redraw = true;
    }

    if (redraw)
    {
        // Force the Viewport to redraw
        unsigned int nViewports = getViewerDelegate()->getNumberOfViewports();
        for (unsigned int i = 0; i < nViewports; ++i)
        {
            getViewerDelegate()->getViewport(i)->setDirty(true);
        }
    }

    return false;
}

void HydraComponent::locationsSelected(
    const std::vector<std::string>& locationPaths)
{
    // Clear selection
    ViewerLocation* root = m_tree.getRoot();
    if (!root) { return; }
    root->setSelected(false, true);

    // Highlight all the Rprims below the selected locations.
    SdfPathVector sdfPaths;
    for (const auto& locationPath : locationPaths)
    {
        ViewerLocation* location = m_tree.get(locationPath);

        // Check if the location is in the tree cache, and consequently starts
        // with "/root". Then this sets the highlighting of all the RPrims under
        // this location.
        if (location)
        {
            location->setSelected(true, false);

            // Add the root location to the rprims list
            SdfPath sdfPath(locationPath);
            sdfPaths.push_back(sdfPath);

            // Iterate all the descendant rprims and add them to the list
            SdfPathVector const& rprimPaths =
                m_hydraKatana->getRenderIndex()->GetRprimSubtree(sdfPath);
            for (const auto& rprimPath : rprimPaths)
            {
                if (rprimPath.IsPropertyPath()) { continue; }
                sdfPaths.push_back(rprimPath);
            }
        }
    }

    // Set selection in Hydra
    m_hydraKatana->select(sdfPaths, true);

    // Mark all viewports as dirty
    unsigned int nViewports = getViewerDelegate()->getNumberOfViewports();
    for (unsigned int i = 0; i < nViewports; ++i)
    {
        getViewerDelegate()->getViewport(i)->setDirty(true);
    }
}
