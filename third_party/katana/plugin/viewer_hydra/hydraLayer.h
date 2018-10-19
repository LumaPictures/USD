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

#ifndef HYDRA_VIEWPORT_LAYER_H_
#define HYDRA_VIEWPORT_LAYER_H_

#include "hydraComponent.h"

#include "hydraKatana.h"

#include <FnAttribute/FnAttribute.h>
#include <FnViewer/plugin/FnEventWrapper.h>
#include <FnViewer/plugin/FnViewportLayer.h>
#include <FnViewer/plugin/FnViewport.h>
#include <FnViewer/plugin/FnViewerDelegateComponent.h>
#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace Foundry::Katana::ViewerUtils;
using namespace Foundry::Katana::ViewerAPI;

/**
 * @brief The ViewportLayer that renders locations of the type "usd".
 *
 * This works in tandem with HydraComponent. It retrieves the HydraKatana
 * that is instantiated and populated by HydraComponent and renders its content.
 * The render configurations will be mostly dictated by the standard Hydra setup
 * in HydraKatana.
 */
class HydraLayer : public ViewportLayer
{
public:
    /** @brief Constructor. */
    HydraLayer();

    /** @brief Destructor. */
    virtual ~HydraLayer();

    /** @brief Returns a new instance of HydraLayer. */
    static ViewportLayer* create()
    {
        return new HydraLayer();
    }

    /**
     * @brief Initializes this layer.
     *
     * Runs immediately before the first drawing. Sets up the HydraKatana 
     * responsible for rendering the scene.
     */
    void setup();

    /** @brief Draws the contents of the Render Index in HydraKatana. */
    void draw();

    /** @brief Implements location picking */
    bool customPick(unsigned int x, unsigned int y,
                    unsigned int w, unsigned int h,
                    bool deepPicking, PickedAttrsMap& pickedAttrs,
                    float* singlePointDepth);

    /** Standard ViewportLayer methods - not used, but declared. */
    bool event(const FnEventWrapper& eventData);
    void resize(unsigned int width, unsigned int height);
    void freeze() {}
    void thaw() {}
    void cleanup() {}

private:
    /// Reference to the VDC that holds the HydraKatana.
    HydraComponent* m_hydraComponent;

    /// Reference to the HydraKatana instance - so that it doesn't have to call
    /// m_hydraComponent->getHydraKatana() on every draw() call.
    HydraKatanaPtr m_hydraKatana;
};


#endif /* HYDRA_VIEWPORT_LAYER_H_ */
