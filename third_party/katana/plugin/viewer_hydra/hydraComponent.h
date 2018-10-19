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

#ifndef HYDRA_COMPONENT_H_
#define HYDRA_COMPONENT_H_

#include "hydraKatana.h"
#include "viewerUtils/viewerLocation.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include <FnViewer/plugin/FnViewerDelegateComponent.h>
#include <FnViewer/plugin/FnViewerDelegate.h>
#include <FnViewer/plugin/FnViewport.h>
#include <FnViewer/plugin/FnMathTypes.h>
#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace Foundry::Katana::ViewerAPI;

/**
 * @brief The VDC that manages the HydraKatana instance.
 *
 * This also maintains a ViewerLocationTree containing ViewerLocation nodes that
 * hold Hydra-related objects associated with the cooked locations (example:
 * different SceneDelegates per location, if needed).
 *
 * This VDC is agnostic in terms of what specific location data should be stored.
 * That is the responsibility of other VDCs that hold more location specidic
 * data and behaviour. The Render Index HydraKatana maintained by this VDC is
 * meant to be populated by these other VDCs and rendered by a single Viewport
 * Layer: the HyraLayer.
 */
class HydraComponent : public ViewerDelegateComponent
{

public:

    /** @brief Constructor. */
    HydraComponent();

    /** @brief Destructor. */
    virtual ~HydraComponent();

    /** @brief Factory Method. */
    static Foundry::Katana::ViewerAPI::ViewerDelegateComponent* create();

    /** @brief Flushes caches */
    static void flush();

    /** @brief Initializes the ViewportDelegateComponent's resources. */
    virtual void setup() {}

    /** @brief Cleans up the ViewportDelegateComponent's resources. */
    virtual void cleanup() {}

    /**
     * @brief Called when a location is created/updated/removed.
     *
     * This adds the location information to the ViewerLocationTree. If the
     * location contains GL guides then the appropriate GuideLocationData will
     * be set on its ViewerLocation.
     *
     * Locations that are hidden by an ancestor that is closed is considered to
     * be removed by the Viewer.
     */
    bool locationEvent(const ViewerLocationEvent& event, bool locationHandled);
    
    /**
     * @brief Called when the location selection changes in Katana.
     *
     * Updates the selection flag on the corresponding ViewerLocation objects
     * and marks all the RPrims of these locaitons to be highlighted when
     * rendered. This will ignore any location that does not start with "/root".
     */
    void locationsSelected(const std::vector<std::string>& locationPaths);

    /**
     * @brief Gets the HydraKatana object, used by HydraLayer to render.
     */
    HydraKatanaPtr getHydraKatana() { return m_hydraKatana; }

    /**
     * @brief Gets the ViewerLocationTree that holds the location data.
     *
     * This is called by HydraLayer in order to draw the Render Index's scene
     * and by other VDCs in order for them to populate the Render Index with
     * RPrims.
     */
    ViewerLocationTree* getTree() { return &m_tree; }

private:
    
    /// The HydraKatana instance
    HydraKatanaPtr m_hydraKatana;

    /// The tree structure that holds the cooked location structure  and the
    /// specific data populated by other VDCs.
    ViewerLocationTree m_tree;
};


#endif
