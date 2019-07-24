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
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hdx/ambientOcclusionTask.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/utils.h"

#include <random>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (hdxAoKernel)
    (hdxAoNumSamples)
    (hdxAoRadius)
    (hdxAoUniforms)
    (hdxAoUniformBar)
    (hdxAoProjectionMatrix)
    (hdxAoProjectionMatrixInv)
    (hdxAoNearFar)
);

namespace {

using HdBufferSourceSharedPtrVector = std::vector<HdBufferSourceSharedPtr>;

class HdxAmbientOcclusionRenderPassShader : public HdStRenderPassShader {
public:
    HdxAmbientOcclusionRenderPassShader() :
        HdStRenderPassShader(HdxPackageAmbientOcclusionImageShader())
    {
        // The hash of this shader is constant, no custom bindings and the
        // input parameters are constant.
        _hash = HdStRenderPassShader::ComputeHash();
    }

    ID ComputeHash() const override
    {
        return _hash;
    }

    void BindResources(const HdSt_ResourceBinder& binder, int program) override
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _depthTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _normalTex);
        HdStRenderPassShader::BindResources(binder, program);
    }

    inline void SetDepthTexture(GLuint tex)
    {
        _depthTex = tex;
    }

    inline void SetNormalTexture(GLuint tex)
    {
        _normalTex = tex;
    }

    ~HdxAmbientOcclusionRenderPassShader() override = default;
private:
    HdxAmbientOcclusionRenderPassShader(const HdxAmbientOcclusionRenderPassShader&)             = delete;
    HdxAmbientOcclusionRenderPassShader(HdxAmbientOcclusionRenderPassShader&&)                  = delete;
    HdxAmbientOcclusionRenderPassShader& operator=(const  HdxAmbientOcclusionRenderPassShader&) = delete;
    HdxAmbientOcclusionRenderPassShader& operator=(HdxAmbientOcclusionRenderPassShader&&)       = delete;

    ID _hash;
    int _depthTex;
    int _normalTex;
};

// The sample should conform to poission disc sampling.
// Once we have the normal available, this becomes a bit easier.
VtArray<GfVec3f> _GenerateSamplingKernel(const int numPoints)
{
    VtArray<GfVec3f> ret; ret.reserve(numPoints);
    std::ranlux24 engineX(42);
    std::ranlux24 engineY(137);
    std::ranlux24 engineZ(1337);

    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    for (auto i = decltype(numPoints){0}; i < numPoints; i += 1) {
        GfVec3f vec {
            distribution(engineX) * 2.0f - 1.0f,
            distribution(engineY) * 2.0f - 1.0f,
            distribution(engineZ),
        };

        vec.Normalize();
        float scale = static_cast<float>(i) / static_cast<float>(numPoints);
        // Lerp between 0.1 and 1.0 for better distribution.
        scale = 0.1 + scale * scale * 0.9;
        vec *= scale;
        ret.push_back(vec);
    }

    return ret;
}

}

HdxAmbientOcclusionTask::HdxAmbientOcclusionTask(
    HdSceneDelegate* delegate, const SdfPath& id) : HdTask(id)
{

}

HdxAmbientOcclusionTask::~HdxAmbientOcclusionTask()
{

}

void HdxAmbientOcclusionTask::Sync(HdSceneDelegate* delegate,
          HdTaskContext* ctx,
          HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    if (*dirtyBits & HdChangeTracker::DirtyParams) {
        auto value = delegate->Get(GetId(), HdTokens->params);
        if (value.IsHolding<HdxAmbientOcclusionTaskParams>()) {
            auto params = value.UncheckedGet<HdxAmbientOcclusionTaskParams>();
            _cameraId = params.cameraId;
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void HdxAmbientOcclusionTask::Prepare(HdTaskContext* ctx,
                                      HdRenderIndex* renderIndex)
{
    auto* renderDelegate = renderIndex->GetRenderDelegate();
    if (!TF_VERIFY(dynamic_cast<HdStRenderDelegate*>(renderDelegate),
                   "OIT Task only works with HdSt")) {
        return;
    }

    auto enableAo = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->enableAo);
    if (!TF_VERIFY(enableAo.IsHolding<bool>(),
                   "Enable Ambient Occlusion is not a bool!")) {
        return;
    }
    if (!TF_VERIFY(enableAo.UncheckedGet<bool>(),
                   "Enable Ambient Occlusion is false, "
                   "yet the task is running.")) {
        return;
    }

    auto aoNumSamplesVal = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->aoNumSamples);
    if (!TF_VERIFY(aoNumSamplesVal.IsHolding<int>(),
                   "Ambient Occlusion num samples is not an integer!")) {
        return;
    }
    const auto aoNumSamples = std::max(1, aoNumSamplesVal.UncheckedGet<int>());
    const auto aoRadiusVal = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->aoRadius);
    if (!TF_VERIFY(aoRadiusVal.IsHolding<float>(),
                   "Ambient Occlusion radius is not a float!")) {
        return;
    }
    const auto aoRadius = std::max(0.0f, aoRadiusVal.UncheckedGet<float>());

    const auto* camera = static_cast<const HdCamera*>(
        renderIndex->GetSprim(HdPrimTypeTokens->camera, _cameraId));
    GfMatrix4f cameraProjection(0.0f);
    if (camera != nullptr) {
        cameraProjection = GfMatrix4f(camera->GetProjectionMatrix());
    }

    auto updateConstants = false;
    auto rebuildKernel = false;
    if (aoNumSamples != _aoNumSamples) {
        _aoNumSamples = aoNumSamples;
        rebuildKernel = true;
        updateConstants = true;
    }

    if (aoRadius != _aoRadius
     || cameraProjection != _cameraProjection) {
        _aoRadius = aoRadius;
        _cameraProjection = cameraProjection;
        updateConstants = true;
    }

    const auto& resourceRegistry = renderIndex->GetResourceRegistry();

    if (!_renderPass) {
        HdRprimCollection collection;

        _renderPass.reset(
            new HdSt_ImageShaderRenderPass(renderIndex, collection));

        // To avoid having to access the color buffer for manipuation, use
        // OpenGL's blending pipeline to multiply the color buffer with the
        // alpha value of our image shader, which is the inverse of the
        // ambient occlusion factor.
        _renderPassState.reset(new HdStRenderPassState());
        _renderPassState->SetEnableDepthMask(false);
        _renderPassState->SetColorMask(HdRenderPassState::ColorMaskRGBA);
        _renderPassState->SetBlendEnabled(true);
        _renderPassState->SetBlend(
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOneMinusSrcAlpha,
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOne);

        _renderPassShader.reset(new HdxAmbientOcclusionRenderPassShader());

        _renderPassState->SetRenderPassShader(_renderPassShader);
        _renderPass->Prepare(GetRenderTags());

        HdBufferSpecVector kernelSpecs;
        kernelSpecs.push_back(
            HdBufferSpec(
                _tokens->hdxAoKernel,
                HdTupleType {HdTypeFloatVec3, 1})
        );

        _kernelBar = resourceRegistry->AllocateSingleBufferArrayRange(
            _tokens->hdxAoKernel,
            kernelSpecs,
            HdBufferArrayUsageHint()
        );

        _renderPassShader->AddBufferBinding(
            HdBindingRequest(
                HdBinding::SSBO,
                _tokens->hdxAoKernel,
                _kernelBar,
                false /* interleave */
            )
        );

        HdBufferSpecVector uniformSpecs;
        uniformSpecs.reserve(5);
        uniformSpecs.emplace_back(
            _tokens->hdxAoNumSamples,
            HdTupleType { HdTypeInt32, 1}
        );
        uniformSpecs.emplace_back(
            _tokens->hdxAoRadius,
            HdTupleType { HdTypeFloat, 1}
        );
        uniformSpecs.emplace_back(
            _tokens->hdxAoProjectionMatrix,
            HdTupleType { HdTypeFloatMat4, 1}
        );
        uniformSpecs.emplace_back(
            _tokens->hdxAoProjectionMatrixInv,
            HdTupleType { HdTypeFloatMat4, 1}
        );
        uniformSpecs.emplace_back(
            _tokens->hdxAoNearFar,
            HdTupleType { HdTypeFloatVec2, 1}
        );

        _uniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
            _tokens->hdxAoUniforms,
            uniformSpecs,
            HdBufferArrayUsageHint()
        );

        _renderPassShader->AddBufferBinding(
            HdBindingRequest(
                HdBinding::UBO,
                _tokens->hdxAoUniformBar,
                _uniformBar,
                true
            )
        );

        rebuildKernel = true;
        updateConstants = true;
    }

    if (rebuildKernel) {
        HdBufferSourceSharedPtr kernelSource(
            new HdVtBufferSource(
                _tokens->hdxAoKernel,
                VtValue(_GenerateSamplingKernel(_aoNumSamples))
            )
        );
        resourceRegistry->AddSource(_kernelBar, kernelSource);
    }

    if (updateConstants) {
        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.reserve(5);
        uniformSources.emplace_back(
            new HdVtBufferSource(_tokens->hdxAoNumSamples, VtValue(_aoNumSamples))
        );
        uniformSources.emplace_back(
            new HdVtBufferSource(_tokens->hdxAoRadius, VtValue(_aoRadius))
        );
        uniformSources.emplace_back(
            new HdVtBufferSource(_tokens->hdxAoProjectionMatrix
                               , VtValue(_cameraProjection))
        );
        uniformSources.emplace_back(
            new HdVtBufferSource(_tokens->hdxAoProjectionMatrixInv
                               , VtValue(_cameraProjection.GetInverse()))
        );
        // http://dougrogers.blogspot.com/2013/02/how-to-derive-near-and-far-clip-plane.html
        // Needed to flip second coordinate, different matrix major.
        const auto C = _cameraProjection[2][2];
        const auto D = _cameraProjection[3][2];
        GfVec2f nearFar {D / (C - 1.0f), D / (C + 1.0f)};
        uniformSources.emplace_back(
            new HdVtBufferSource(_tokens->hdxAoNearFar
                               , VtValue(nearFar))
        );
        resourceRegistry->AddSources(_uniformBar, uniformSources);
    }
}

void HdxAmbientOcclusionTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_renderPassState)) return;

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION
                   , 0
                   , -1
                   , "Ambient Occlusion Rendering");

    GLint drawFramebuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);

    const auto screenSize = HdxUtils::GetScreenSize();

    auto drawTarget = GlfDrawTarget::New(screenSize
                                       , false /* request MSAA */);

    drawTarget->Bind();
    drawTarget->AddAttachment("depth"
                            , GL_DEPTH_COMPONENT
                            , GL_FLOAT
                            , GL_DEPTH_COMPONENT32F);
    drawTarget->AddAttachment("normal"
                            , GL_RGBA
                            , GL_FLOAT
                            , GL_RGBA16F);
    drawTarget->DrawBuffers();

    auto framebuffer = drawTarget->GetFramebufferId();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    // Normal is bound to the second slot. Luckily blit framebuffer only
    // blits the read buffer to the drawbuffer(s), so we don't have to keep
    // a color buffer around.
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, screenSize[0], screenSize[1],
                      0, 0, screenSize[0], screenSize[1],
                      GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
    GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, buffers);
    GLF_POST_PENDING_GL_ERRORS();

    auto* shader = static_cast<HdxAmbientOcclusionRenderPassShader*>(
        _renderPassShader.get());
    shader->SetDepthTexture(drawTarget->GetAttachment("depth")->GetGlTextureName());
    shader->SetNormalTexture(drawTarget->GetAttachment("normal")->GetGlTextureName());

    _renderPassState->Bind();

    glDisable(GL_DEPTH_TEST);

    _renderPass->Execute(_renderPassState, GetRenderTags());

    glEnable(GL_DEPTH_TEST);

    _renderPassState->Unbind();

    drawTarget->Bind();
    drawTarget->ClearAttachments();
    drawTarget = nullptr;
    glPopDebugGroup();
}

std::ostream& operator<<(
    std::ostream& out, const HdxAmbientOcclusionTaskParams& pv)
{
    out << "AmbientOcclusionTask Params: (...) "
        << pv.enable;
    return out;
}

bool operator==(
    const HdxAmbientOcclusionTaskParams& lhs,
    const HdxAmbientOcclusionTaskParams& rhs)
{
    return lhs.enable == rhs.enable;
}

bool operator!=(
    const HdxAmbientOcclusionTaskParams& lhs,
    const HdxAmbientOcclusionTaskParams& rhs)
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
