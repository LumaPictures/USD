//
// Copyright 2019 Pixar
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
#include "pxr/imaging/glf/glew.h"

#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/oitResolveTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/utils.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDX_ENABLE_OIT, true, 
                      "Enable order independent translucency");

namespace {

decltype(glClearNamedBufferData) _GetGlClearNamedBufferData() {
    return glClearNamedBufferData ?
           glClearNamedBufferData :
           glClearNamedBufferDataEXT;
}

}

typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceSharedPtrVector;

// -------------------------------------------------------------------------- //

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader()
    , _oitOpaqueRenderPassShader()
    , _bufferSize(0)
    , _screenSize(1,1)
{
    _oitTranslucentRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassOitShader()));

    _oitOpaqueRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassOitOpaqueShader()));
}

HdxOitRenderTask::~HdxOitRenderTask()
{
}

void
HdxOitRenderTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    HdxRenderTask::Sync(delegate, ctx, dirtyBits);
}

void
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    HdxRenderTask::Prepare(ctx, renderIndex);
    _PrepareOitBuffers(ctx, renderIndex); 
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    //
    // Pre Execute Setup
    //

    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(renderPassState)) return;

    HdStRenderPassState* extendedState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
        return;
    }

    extendedState->SetOverrideShader(HdStShaderCodeSharedPtr());

    _ClearOitGpuBuffers(ctx);

    // We render into a SSBO -- not MSSA compatible
    bool oldMSAA = glIsEnabled(GL_MULTISAMPLE);
    glDisable(GL_MULTISAMPLE);
    // XXX When rendering HdStPoints we set GL_POINTS and assume that
    //     GL_POINT_SMOOTH is enabled by default. This renders circles instead
    //     of squares. However, when toggling MSAA off (above) we see GL_POINTS
    //     start to render squares (driver bug?).
    //     For now we always enable GL_POINT_SMOOTH. 
    // XXX Switch points rendering to emit quad with FS that draws circle.
    bool oldPointSmooth = glIsEnabled(GL_POINT_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    //
    // Opaque pixels pass
    // These pixels are rendered to FB instead of OIT buffers
    //
    extendedState->SetRenderPassShader(_oitOpaqueRenderPassShader);
    renderPassState->SetEnableDepthMask(true);
    renderPassState->SetColorMask(HdRenderPassState::ColorMaskRGBA);
    HdxRenderTask::Execute(ctx);

    //
    // Translucent pixels pass
    //
    extendedState->SetRenderPassShader(_oitTranslucentRenderPassShader);
    renderPassState->SetEnableDepthMask(false);
    renderPassState->SetColorMask(HdRenderPassState::ColorMaskNone);
    HdxRenderTask::Execute(ctx);

    //
    // Post Execute Restore
    //

    if (oldMSAA) {
        glEnable(GL_MULTISAMPLE);
    }

    if (!oldPointSmooth) {
        glDisable(GL_POINT_SMOOTH);
    }
}

void
HdxOitRenderTask::_PrepareOitBuffers(
    HdTaskContext* ctx, 
    HdRenderIndex* renderIndex)
{
    // XXX OIT can be globally disabled to preserve GPU memory
    if (!bool(TfGetEnvSetting(HDX_ENABLE_OIT))) return;

    HdRenderDelegate* renderDelegate = renderIndex->GetRenderDelegate();
    if (!TF_VERIFY(dynamic_cast<HdStRenderDelegate*>(renderDelegate),
                   "OIT Task only works with HdSt")) {
        return;
    }

    // XXX Exit if opengl version too old
    if (!glGetTextureLevelParameteriv) return;

    HdResourceRegistrySharedPtr const& resourceRegistry = 
        renderIndex->GetResourceRegistry();

    GfVec2i s = HdxUtils::GetScreenSize();
    bool screenChanged = s != _screenSize;
    _screenSize = s;

    int newBufferSize = _screenSize[0] * _screenSize[1];
    bool rebuildOitBuffers = (newBufferSize != _bufferSize);

    VtValue oitNumSamples = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->oitNumSamples);
    if (!TF_VERIFY(oitNumSamples.IsHolding<int>(),
        "OIT Number of Samples is not an integer!")) {
        return;
    }
    const int numSamples = std::max(1, oitNumSamples.UncheckedGet<int>());
    if (_numSamples != numSamples) {
        rebuildOitBuffers = true;
        _numSamples = numSamples;
    }

    if (rebuildOitBuffers) {
        // If glew version too old we emit a warning since OIT will not work.
        if (_GetGlClearNamedBufferData() == nullptr) {
            TF_WARN("glClearNamedBufferData missing for OIT (old glew?)");
        }

        _counterBar.reset();
        _dataBar.reset();
        _depthBar.reset();
        _indexBar.reset();
        _bufferSize = newBufferSize;
        renderIndex->GetChangeTracker().SetGarbageCollectionNeeded();
    }

    if (screenChanged) {
        _uniformBar.reset();
        renderIndex->GetChangeTracker().SetGarbageCollectionNeeded();
    }

    //
    // Counter Buffer
    //
    if (!_counterBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitCounterBuffer, 
            HdTupleType {HdTypeInt32, 1}));
        _counterBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitCounter,
                                            specs,
                                            HdBufferArrayUsageHint());
        _counterBar->Resize(newBufferSize + 1);
    }

    (*ctx)[HdxTokens->oitCounterBufferBar] = _counterBar;

    //
    // Index Buffer
    //
    if (!_indexBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitIndexBuffer,
            HdTupleType {HdTypeInt32, 1}));
        _indexBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitIndices,
                                            specs,
                                            HdBufferArrayUsageHint());
        _indexBar->Resize(newBufferSize * numSamples);
    }

    (*ctx)[HdxTokens->oitIndexBufferBar] = _indexBar;

    //
    // Data Buffer
    //
    if (!_dataBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDataBuffer, 
            HdTupleType {HdTypeFloatVec4, 1}));
        _dataBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitData,
                                            specs,
                                            HdBufferArrayUsageHint());
        _dataBar->Resize(newBufferSize * numSamples);
    }

    (*ctx)[HdxTokens->oitDataBufferBar] = _dataBar;

    //
    // Depth Buffer
    //
    if (!_depthBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDepthBuffer, 
            HdTupleType {HdTypeFloat, 1}));
        _depthBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitDepth,
                                            specs,
                                            HdBufferArrayUsageHint());
        _depthBar->Resize(newBufferSize * numSamples);
    }

    (*ctx)[HdxTokens->oitDepthBufferBar] = _depthBar;

    //
    // Uniforms
    //
    if (!_uniformBar) {
        HdBufferSpecVector specs;
        specs.push_back( HdBufferSpec(
            HdxTokens->oitBufferSize, HdTupleType {HdTypeInt32, 1}));
        specs.push_back( HdBufferSpec(
            HdxTokens->oitScreenSize,HdTupleType{HdTypeInt32Vec2, 1}));
        specs.push_back( HdBufferSpec(
            HdxTokens->oitNumSamples, HdTupleType {HdTypeInt32, 1}));

        _uniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
                                            /*role*/HdxTokens->oitUniforms,
                                            specs,
                                            HdBufferArrayUsageHint());

        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->oitBufferSize,
                                    VtValue(newBufferSize*numSamples))));
        uniformSources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(HdxTokens->oitScreenSize,
                                                   VtValue(_screenSize))));
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->oitNumSamples,
                                     VtValue(_numSamples))));
        resourceRegistry->AddSources(_uniformBar, uniformSources);
    }

    (*ctx)[HdxTokens->oitUniformBar] = _uniformBar;

    //
    // Binding Requests
    //
    if (rebuildOitBuffers) {
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitCounterBufferBar, _counterBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDataBufferBar, _dataBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDepthBufferBar, _depthBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitIndexBufferBar, _indexBar,
                             /*interleave*/false));
    }

    if (screenChanged) {
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->oitUniformBar, _uniformBar,
                             /*interleave*/true));
    }
}

void 
HdxOitRenderTask::_ClearOitGpuBuffers(HdTaskContext* ctx)
{
    auto clearFunc = _GetGlClearNamedBufferData();
    // Exit if glew version used by app is too old
    if (clearFunc == nullptr || _counterBar == nullptr) return;

    //
    // Counter Buffer
    //
    HdStBufferArrayRangeGLSharedPtr stCounterBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_counterBar);
    HdStBufferResourceGLSharedPtr stCounterResource = 
        stCounterBar->GetResource(HdxTokens->hdxOitCounterBuffer);

    const GLint clearCounter = -1;
    clearFunc(stCounterResource->GetId(),
              GL_R32I,
              GL_RED_INTEGER,
              GL_INT,
              &clearCounter);
}


PXR_NAMESPACE_CLOSE_SCOPE
