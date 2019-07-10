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

#include "hydraLayer.h"

#include <FnViewer/plugin/FnViewport.h>
#include <FnViewer/plugin/FnViewportCamera.h>
#include <FnViewer/utils/FnDrawingHelpers.h>
#include <FnViewer/utils/FnImathHelpers.h>
#include <FnViewer/plugin/FnGLStateHelper.h>

#include <OpenEXR/ImathMatrix.h>

#include <algorithm>

FnLogSetup("Viewer.Hydra.ViewportLayer");

using namespace IMATH_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE


HydraLayer::HydraLayer() : m_hydraComponent(NULL) {}

HydraLayer::~HydraLayer() {}

void HydraLayer::setup()
{
    // Get the ViewerDelegateComponent
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

    // Setup the Katana Hydra instance
    m_hydraKatana->setup();
}

void HydraLayer::draw()
{
    if (!m_hydraKatana || !m_hydraKatana->isReadyToRender()) { return; }

    m_hydraKatana->draw(getViewport());
}

bool HydraLayer::customPick(unsigned int x, unsigned int y,
                          unsigned int w, unsigned int h,
                          bool deepPicking,
                          PickedAttrsMap& pickedAttrs,
                          float* singlePointDepth)
{
    // Perform the picking
    HdxPickHitVector hits;
    if (!m_hydraKatana->pick(getViewport(), x, y, w, h, deepPicking, hits))
    {
        return true;
    }

    // Set the Single Point Depth using the first hit, if it is requested
    if (singlePointDepth)
    {
        *singlePointDepth = hits.begin()->ndcDepth;
    }

    // The hits can be repeated, so get a non-repeating set of the hit locations
    std::set<std::string> locationPaths;
    for (const auto& hit : hits)
    {
        std::string locationPath = hit.objectId.GetString();

        // Get the first non virtual ancestor above
        ViewerLocation* location =
            m_hydraComponent->getTree()->findNearestAncestor(locationPath, true);

        if (location)
        {
            locationPaths.insert(location->getPath());
        }
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

bool HydraLayer::event(const FnEventWrapper& eventData)
{
    return false;
}

void HydraLayer::resize(unsigned int width, unsigned int height) {}

