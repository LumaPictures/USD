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
#include "hydraKatana.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

#include <FnViewerModifier/plugin/FnViewerModifier.h>
#include <FnViewer/plugin/FnGLStateHelper.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(KATANA_HYDRA,
                "Hydra instance to be used by Katana Viewer Plugins.");
}


HydraKatana::HydraKatana()
    : m_taskController(nullptr),
      m_selectionColor(1, 1, 1, 1)
{
    // Initialize the Render Delegate and the Render Index
    m_renderDelegate = new HdStRenderDelegate();
    m_renderIndex = HdRenderIndex::New(m_renderDelegate);
}

HydraKatana::~HydraKatana()
{
    if (m_renderIndex != nullptr)
    {
        delete m_renderIndex;
    }

    if (m_renderDelegate != nullptr)
    {
        delete m_renderDelegate;
    }
}

HydraKatanaPtr HydraKatana::Create()
{
    return std::make_shared<HydraKatana>();
}

HdRenderIndex* HydraKatana::getRenderIndex()
{
    return m_renderIndex;
}

void HydraKatana::setup()
{
    if (isReadyToRender())
    {
        TF_DEBUG(KATANA_HYDRA).Msg("Katana Hydra already set up");
        return;
    }

    if(!m_renderDelegate || ! m_renderIndex)
    {
        TF_DEBUG(KATANA_HYDRA).Msg("Hydra Render Index not Initialized");
        return;
    }

    // Check the GL context and Hydra
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!context) {
        TF_DEBUG(KATANA_HYDRA).Msg(
            "OpenGL context required, using reference renderer");
        return;
    }

    GlfContextCaps::InitInstance();

    if (!HdStRenderDelegate::IsSupported())
    {
        TF_DEBUG(KATANA_HYDRA).Msg(
                "Current GL context doesn't support Hydra");
        return;
    }

    if (TfGetenv("HD_ENABLED", "1") != "1")
    {
        TF_DEBUG(KATANA_HYDRA).Msg("HD_ENABLED not enabled.");
        return;
    }

    // Make the GL Context current
    GlfGLContext::MakeCurrent(context);

    // Init GLEW
    GlfGlewInit();

    // Create the Task controller
    m_taskController = new HdxTaskController(m_renderIndex,
        SdfPath("/KatanaHydra_TaskController"));
    m_taskController->SetEnableSelection(true);
    m_taskController->SetSelectionColor(m_selectionColor);

    // Task Params
    HdxRenderTaskParams renderTaskParams;
    renderTaskParams.enableLighting = true;
    m_taskController->SetRenderParams(renderTaskParams);

    // Render Tags
    TfTokenVector renderTags;
    renderTags.push_back(HdRenderTagTokens->geometry);
    renderTags.push_back(HdRenderTagTokens->proxy);
    // NOTE: in order to render in full res use this instead of 
    // HdRenderTagTokens->proxy: 
    // renderTags.push_back(HdRenderTagTokens->render);
    m_taskController->SetRenderTags(renderTags);

    // Selection Tracker
    m_selectionTracker.reset(new HdxSelectionTracker());

    // Lighting Context
    m_lightingContext = initLighting();

    // Create the collection with all the geometry
    m_geoCollection = HdRprimCollection(
        TfToken("katanaHydraGeo"),
        HdReprSelector(HdReprTokens->smoothHull));
    m_geoCollection.SetRootPath(SdfPath::AbsoluteRootPath());

    // Set the Collection in various entities
    m_taskController->SetCollection(m_geoCollection);
    m_renderIndex->GetChangeTracker().AddCollection(m_geoCollection.GetName());
}

void HydraKatana::draw(ViewportWrapperPtr viewport)
{
    if (!isReadyToRender()) { return; }

    // Currently needed. According to @mwdd:
    //   """
    //   HdxTaskController::_Delegate::IsEnabled() is what is forcing you to do
    //   it. If that returned false, the value in HdxRenderTaskParams, would be
    //   used.  We should fix that!
    //   """
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // Set the Lighting State
    m_taskController->SetLightingState(m_lightingContext);

    // Camera
    auto camera = viewport->getActiveCamera();

    // Camera Matrices
    GfMatrix4d projMatrix = toGfMatrixd(camera->getProjectionMatrix());
    GfMatrix4d viewMatrix = toGfMatrixd(camera->getViewMatrix());
    m_taskController->SetFreeCameraMatrices(viewMatrix, projMatrix);

    // Viewport size
    GfVec4d glviewport(0, 0, viewport->getWidth(), viewport->getHeight());
    m_taskController->SetRenderViewport(glviewport);

    // Sync collection with viewer display mode
    syncDisplayMode(viewport);

    // Engine Selection State
    VtValue selectionValue(m_selectionTracker);
    m_engine.SetTaskContextData(HdxTokens->selectionState, selectionValue);

    // Render
    auto tasks = m_taskController->GetRenderingTasks();
    m_engine.Execute(m_renderIndex, &tasks);
}


bool HydraKatana::pick(ViewportWrapperPtr viewport,
    unsigned int x, unsigned int y, unsigned int w, unsigned int h,
    bool deepPicking, HdxPickHitVector& hits)
{
    if (!isReadyToRender()) { return false; }

    // Make GL context current
    GlfGLContext::MakeCurrent(GlfGLContext::GetCurrentGLContext());

    HdxPickTaskContextParams pickParams;
    pickParams.outHits = &hits;

    // Define the hit mode
    if (deepPicking) {
        pickParams.hitMode = HdxPickTokens->hitAll;
        pickParams.resolveMode = HdxPickTokens->resolveAll;
    } else {
        pickParams.hitMode = HdxPickTokens->hitFirst;
        pickParams.resolveMode = HdxPickTokens->resolveNearestToCenter;
    }

    // Get the Viewport dimensions in pixels
    const int viewportWidth = viewport->getWidth();
    const int viewportHeight = viewport->getHeight();
    if (viewportWidth <= 0 || viewportHeight <= 0) { return false; }

    // Get the View and Projection Matrices for the area frustum
    const Imath::Matrix44<double> projectionMat = getFrustumFromRect(x, y, w, h,
        viewportWidth, viewportHeight, viewport->getProjectionMatrix());

    pickParams.projectionMatrix = toGfMatrixd(projectionMat.getValue());
    pickParams.viewMatrix = toGfMatrixd(viewport->getViewMatrix());

    // Sync collection with viewer display mode
    syncDisplayMode(viewport);

    pickParams.collection = m_geoCollection;

    // Get the hit objects
    VtValue vtPickParams(pickParams);
    m_engine.SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);

    // Execute the picking tasks
    auto tasks = m_taskController->GetPickingTasks();
    m_engine.Execute(m_renderIndex, &tasks);

    // Hydra resizes the viewport to 128x128. We had to reset it back.
    glViewport(0, 0, viewportWidth, viewportHeight);

    return hits.size() > 0;
}

void HydraKatana::select(const SdfPathVector& paths, bool replace)
{
	if (!isReadyToRender()) { return; }

    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    HdSelectionSharedPtr selection;

    // If we are adding to the selected paths, rather than replacing the
    // existing ones then start by getting the currently selected paths.
    if (!replace)
    {
        selection = m_selectionTracker->GetSelectionMap();
    }

    // If there are no currenly selected paths yet or if we are replacing the
    // currenly selected ones, then allocat a new empty selection list.
    if (!selection || replace)
    {
        selection = HdSelectionSharedPtr(new HdSelection());
    }

    // Add the paths to the selection list
    for (const auto& path : paths)
    {
        selection->AddRprim(mode, path);
    }

    // Add the paths to the selection tracker
    m_selectionTracker->SetSelection(selection);
}

void HydraKatana::select(const SdfPathSet& paths, bool replace)
{
	if (!isReadyToRender()) { return; }

    SdfPathVector vec;
    vec.assign(paths.begin(), paths.end());
    select(vec, replace);
}

void HydraKatana::setSelectionColor(float r, float g, float b, float a)
{
    m_selectionColor = GfVec4f(r, g, b, a);

    if (m_taskController)
    {
        m_taskController->SetSelectionColor(m_selectionColor);
    }
}

bool HydraKatana::isReadyToRender()
{
	return m_taskController != NULL;
}

const Imath::Matrix44<double> HydraKatana::getFrustumFromRect(
    int x, int y, int w, int h, int viewportWidth, int viewportHeight,
    const double* currentProjMat)
{
    int viewportDimensions[4] = { 0, 0, viewportWidth, viewportHeight };
    double dWidth = static_cast<double>(w);
    double dHeight = static_cast<double>(h);

    // Projection calculation
    double cx = x + w / 2.0f;
    double cy = viewportDimensions[3] - y - h / 2.0f;
    double sx = viewportDimensions[2] / dWidth;
    double sy = viewportDimensions[3] / dHeight;
    double tx = (viewportDimensions[2] + 2.0f * (viewportDimensions[0] - cx));
    double ty = (viewportDimensions[3] + 2.0f * (viewportDimensions[1] - cy));
    tx = tx / dWidth;
    ty = ty / dHeight;

    Imath::Matrix44<double> selectionMatrix(sx, 0, 0, 0,
                                            0, sy, 0, 0,
                                            0, 0, 1, 0,
                                            tx, ty, 0, 1);

    Imath::Matrix44<double> projMatrix =
        FnKat::ViewerUtils::toImathMatrix44d(currentProjMat);

    projMatrix = projMatrix * selectionMatrix;

    return projMatrix;
}

GlfSimpleLightingContextRefPtr HydraKatana::initLighting()
{
    GlfSimpleLightingContextRefPtr context = GlfSimpleLightingContext::New();
    GlfSimpleLightVector lights;

    // Create a camera space light
    GlfSimpleLight light;
    light.SetAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
    light.SetDiffuse(GfVec4f(1.0, 1.0, 1.0, 1.0));
    light.SetSpecular(GfVec4f(0.2, 0.2, 0.2, 1.0));
    light.SetIsCameraSpaceLight(true);
    lights.push_back(light);

    context->SetLights(lights);
    return context;
}

void HydraKatana::syncDisplayMode(ViewportWrapperPtr viewport)
{
    FnAttribute::StringAttribute displayModeAttr =
        viewport->getOption("Global.View.Display Mode");

    if (displayModeAttr == m_displayModeAttr)
    {
        return;
    }

    m_displayModeAttr = displayModeAttr;

    const std::string displayMode = displayModeAttr.getValue("Solid", false);

    TfToken reprType;

    if (displayMode == "Points")
    {
        reprType = HdReprTokens->points;
    }   
    else if (displayMode == "Wireframe")
    {
        reprType = HdReprTokens->wire;
    }
    else if (displayMode == "Flat Shaded")
    {
        reprType = HdReprTokens->hull;
    }
    else // Solid
    {
        reprType = HdReprTokens->smoothHull;
    }

    m_geoCollection.SetReprSelector(HdReprSelector(reprType));
    m_taskController->SetCollection(m_geoCollection);
}

PXR_NAMESPACE_CLOSE_SCOPE
