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

#ifndef USD_LAYER_H_
#define USD_LAYER_H_

#include "hydraComponent.h"
#include "usdComponent.h"

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
 * @brief ViewportLayer that implements the USD location picking.
 *
 * Works in tandem with the companion USDComponent.
 *
 * The option "SelectRprims" option (see getOption() / setOption()) defines if
 * sub-stage RPrim picking is turned on. This can be set by the Viewer/Katana
 * UI, for example.
 *
 * This Layer uses Hydra in customPick() to pick individual RPrims in the scene.
 * If the "SelectRprims" option is on, then this will use USDComponent's sub
 * stage RPrim selection functions, otherwise it will try to figure out which
 * usd location the RPrim corresponds and select it.
 */
class USDLayer : public ViewportLayer
{
public:
    /** @brief Constructor. */
    USDLayer();

    /** @brief Destructor. */
    virtual ~USDLayer();

    /** @brief Returns a new instance of USDLayer. */
    static ViewportLayer* create()
    {
        return new USDLayer();
    }

    /**
     * @brief Initializes this layer.
     *
     * Runs immediately before the first drawing. Gets a reference to
     * USDComponent and HydraKatana.
     */
    void setup() override;

    /**
     * @brief Implements RPrim picking.
     *
     * If the "SelectRprims" option is on, then this will use USDComponent's sub
     * stage RPrim selection functions to select specific the RPrims, otherwise
     * it will figure out which usd location the RPrim corresponds to (via the
     * USDComponent->getSelectedRprims() method) and select it.
     */
    bool customPick(unsigned int x, unsigned int y,
                    unsigned int w, unsigned int h,
                    bool deepPicking, PickedAttrsMap& pickedAttrs,
                    float* singlePointDepth) override;

    /** @brief: Handles UI events */
    bool event(const FnEventWrapper& eventData) override;

    /** Standard ViewportLayer methods - not used, but declared. */
    void draw() override {};
    void resize(unsigned int width, unsigned int height) override {};
    void freeze() override {}
    void thaw() override {}
    void cleanup() override {}

private:
    /// Reference to the VDC that holds the HydraKatana.
    HydraComponent* m_hydraComponent;

    /// Reference to the ViewerDelegateComponent that detects usd locations and
    /// populats the Render Index in HydraKatana with stage RPrims.
    USDComponent* m_usdComponent;
    

    /// Reference to the HydraKatana instance - so that it doesn't have to call
    /// m_hydraComponent->getHydraKatana() on every customPick() call.
    HydraKatanaPtr m_hydraKatana;

    /// Key modifier
    bool m_shiftModifier;
    bool m_ctrlModifier;

    /// Makes sure that the bounds location ends up being selected after the
    /// whole picking process ends.
    void forceBoundsLocationSelection(std::set<std::string>& locationsToSelect);
};


#endif /* USD_LAYER_H_ */
