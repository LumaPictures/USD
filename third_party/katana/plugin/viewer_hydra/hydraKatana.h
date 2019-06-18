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

#ifndef VIEWER_UTILS_HYDRA_KATANA_H
#define VIEWER_UTILS_HYDRA_KATANA_H

#include <GL/glew.h>

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/glf/glContext.h"

#include <FnViewer/plugin/FnViewport.h>
#include <FnViewer/utils/FnImathHelpers.h>

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

using Foundry::Katana::ViewerAPI::ViewportWrapperPtr;

#ifndef toGfMatrixd
    /// Helper macro to convert attribute data to USD format.
    #define toGfMatrixd(data)                                       \
        GfMatrix4d(data[ 0], data[ 1], data[ 2], data[ 3],          \
                   data[ 4], data[ 5], data[ 6], data[ 7],          \
                   data[ 8], data[ 9], data[10], data[11],          \
                   data[12], data[13], data[14], data[15])
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (proxy)
    (render)
);

TF_DEBUG_CODES( 
    KATANA_HYDRA   
);

/// Forward declaration.
class HydraKatana;

/// Ref-counted pointer to a HydraKatana object.
typedef std::shared_ptr<HydraKatana> HydraKatanaPtr;

/** @brief Hydra instance used by Hydra powered Layers and Components.
 *
 * This class holds the necessary Hydra objects to allow Katana Viewer plugins
* (ViewerDelegateComponents and ViewportLayers) to make use of Hydra to render
 * parts of the scene. These objects use a Hydra configuration that will allow
 * different plugins to render in a consistent way. Ideally, a single instance
 * of this object should be used by all Hydra based viewer plugins developed at
 * Pixar. It should be instantiated by a ViewerDelegateComponents and shared
 * with the other plugins (other ViewerDelegateComponents and/or ViewportLayers).
 * 
 * The basic idea is that ViewerDelegateComponents are responsible for
 * populating a HdRenderIndex and the ViewportLayers for rendering it.
 *
 * NOTE: currently this only supports the GL RenderDelegate (HdStRenderDelegate,
 *       see the constructor), but should be changed in the future to support
 *       different render heads - PRman, Rtp, Embree, etc).
 */
class HydraKatana
{
public:

    /** @brief Constructor. */
    HydraKatana();

    /** @brief Destructor. */
    ~HydraKatana();

    /** @brief Creates a reference counted pointer to a new instance.*/
    static HydraKatanaPtr Create();

    /** @brief  Returns the Render Index. */
    HdRenderIndex* getRenderIndex();

    /**
     * @brief Initializes the necessary Hydra objects.
     * 
     * Initializes and configures some Hydra objects used in the rendering of
     * the Render Index. This should be called in a ViewportLayer::setup()
     * method, which runs in the expected GL context.
     */
    void setup();

    /**
     * @brief Draws the scene in the Render Index.
     *
     * Renders the content of the Render Index. This Uses the HdRenderDelegate
     * instantiated in the constructor to render the scene. This should be
     * called in the plugin's ViewportLayer::draw() function.
     */
    void draw(ViewportWrapperPtr viewport);

    /** @brief Uses Hydra to pick the objects in the given viewport area. */
    bool pick(ViewportWrapperPtr viewport,
              unsigned int x, unsigned int y,
              unsigned int w, unsigned int h,
              bool deepPicking,
              HdxPickHitVector& hits);

    /**
     * @brief Highlights the RPrims with the given paths.
     *
     * @param paths: An ordered vector of SdfPaths to be highlighted.
     * @param replace: If true, then clear all highlighted RPrims and hightlight
     *    only the given ones.
     */
    void select(const SdfPathVector& paths, bool replace=true);

    /**
     * @brief Highlights the RPrims with the given paths.
     *
     * Similar to the previous method, but accepts a set of SdfPaths, rather
     * than a vector.
     *
     * @param paths: An non-repeating set of SdfPaths to be highlighted.
     * @param replace: If true, then clear all highlighted RPrims and hightlight
     *    only the given ones.
     */
    void select(const SdfPathSet& paths, bool replace=true);

    void setSelectionColor(float r, float g, float b, float a);

    /** @brief Tells if this has been initialized by setup().
     *
     * Tells if all the objects were properly contructed and setup() did not
     * fail somehow. This should be checked before calling draw().
     */
    bool isReadyToRender();

private:
    /** @brief Builds the projection matrix for the given viewport area. */
    const Imath::Matrix44<double> getFrustumFromRect(
        int x, int y, int w, int h, int viewportWidth, int viewportHeight,
        const double* currentProjMat);

    /** @brief Initializes the default lights. */
    GlfSimpleLightingContextRefPtr initLighting();

    /// The Render Delegate. Currently the constructor will always instantiate
    /// a HdStRenderDelegate for it, but in order to support different renderers
    /// this should be changed.
    HdRenderDelegate* m_renderDelegate;

    /// The Render Index that will be populated in the ViewerDelegateComponent.
    HdRenderIndex*    m_renderIndex;

    /// The different objects used in the rendering of the scene.
    HdEngine                       m_engine;
    HdxTaskController*             m_taskController;
    TfTokenVector                  m_renderTags;
    HdxRenderTaskParams            m_renderTaskParams;
    HdxSelectionTrackerSharedPtr   m_selectionTracker;
    GlfSimpleLightingContextRefPtr m_lightingContext;

    // The selection color
    GfVec4f m_selectionColor;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* VIEWER_UTILS_HYDRA_KATANA_H */
