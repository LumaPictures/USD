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

#include "pxr/imaging/hdx/ambientOcclusionTask.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hdx/package.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class HdxAmbientOcclusionRenderPassShader : public HdStRenderPassShader {
public:
    HdxAmbientOcclusionRenderPassShader(int numSamples) :
        HdStRenderPassShader(HdxPackageAmbientOcclusionImageShader()),
        _numSamples(numSamples)
    {
        // The hash of this shader is constant, no custom bindings and the
        // input parameters are constant.
        _hash = HdStRenderPassShader::ComputeHash();
        boost::hash_combine(_hash, numSamples);
    }

    std::string GetSource(const TfToken& shaderStageKey) const override
    {
        const auto src = HdStRenderPassShader::GetSource(shaderStageKey);

        std::stringstream defines;
        defines << "#define AO_SAMPLES "
                << _numSamples
                << "\n";

        return defines.str() + src;
    }

    ID ComputeHash() const override
    {
        return _hash;
    }

    ~HdxAmbientOcclusionRenderPassShader() override = default;
private:
    HdxAmbientOcclusionRenderPassShader()                                                       = delete;
    HdxAmbientOcclusionRenderPassShader(const HdxAmbientOcclusionRenderPassShader&)             = delete;
    HdxAmbientOcclusionRenderPassShader(HdxAmbientOcclusionRenderPassShader&&)                  = delete;
    HdxAmbientOcclusionRenderPassShader& operator=(const  HdxAmbientOcclusionRenderPassShader&) = delete;
    HdxAmbientOcclusionRenderPassShader& operator=(HdxAmbientOcclusionRenderPassShader&&)       = delete;

    const int _numSamples;
    ID _hash;
};

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
    *dirtyBits = HdChangeTracker::Clean;
}

void HdxAmbientOcclusionTask::Prepare(HdTaskContext* ctx,
                                      HdRenderIndex* renderIndex)
{
    HdRenderDelegate* renderDelegate = renderIndex->GetRenderDelegate();
    if (!TF_VERIFY(dynamic_cast<HdStRenderDelegate*>(renderDelegate),
                   "OIT Task only works with HdSt")) {
        return;
    }

    VtValue aoNumSamples = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->aoNumSamples);
    if (!TF_VERIFY(aoNumSamples.IsHolding<int>(),
                   "Ambient Occlusion num samples is not an integer!")) {
        return;
    }
    const int numSamples = aoNumSamples.UncheckedGet<int>();
    bool rebuildShader = false;
    if (numSamples != _numSamples) {
        _numSamples = numSamples;
        rebuildShader = true;
    }

    if (!_renderPass) {
        HdRprimCollection collection;

        _renderPass = HdRenderPassSharedPtr(
            new HdSt_ImageShaderRenderPass(renderIndex, collection));

        // To avoid having to access the color buffer for manipuation, use
        // OpenGL's blending pipeline to multiply the color buffer with the
        // alpha value of our image shader, which is the inverse of the
        // ambient occlusion factor.
        _renderPassState = boost::make_shared<HdStRenderPassState>();
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

        _renderPassShader.reset(new HdxAmbientOcclusionRenderPassShader(
            8));

        HdStRenderPassState* stRenderPassState =
            static_cast<HdStRenderPassState*>(_renderPassState.get());
        stRenderPassState->SetRenderPassShader(_renderPassShader);
    } else if(rebuildShader) {
        _renderPassShader.reset(new HdxAmbientOcclusionRenderPassShader(
            _numSamples));
        HdStRenderPassState* stRenderPassState =
            static_cast<HdStRenderPassState*>(_renderPassState.get());
        stRenderPassState->SetRenderPassShader(_renderPassShader);
    }
}

void HdxAmbientOcclusionTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_renderPassState)) return;

    // HdStRenderPassState* stRenderPassState =
    //     static_cast<HdStRenderPassState*>(_renderPassState.get());

    _renderPassState->Bind();

    glDisable(GL_DEPTH_TEST);

    _renderPass->Execute(_renderPassState, GetRenderTags());

    glEnable(GL_DEPTH_TEST);

    _renderPassState->Unbind();
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
