#ifndef HDX_AMBIENTOCCLUSION_TASK_H
#define HDX_AMBIENTOCCLUSION_TASK_H

#include "pxr/pxr.h"

#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/glf/drawTarget.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

struct HdxAmbientOcclusionTaskParams {
    SdfPath cameraId;
    bool enable;
};

using HdStRenderPassStateSharedPtr =
    boost::shared_ptr<class HdStRenderPassState>;
using HdStRenderPassShaderSharedPtr =
    boost::shared_ptr<class HdStRenderPassShader>;
using HdSt_ImageShaderRenderPassSharedPtr =
    boost::shared_ptr<class HdSt_ImageShaderRenderPass>;

class HdxAmbientOcclusionTask : public HdTask {
public:
    HDX_API
    HdxAmbientOcclusionTask(HdSceneDelegate* delegate, const SdfPath& id);

    HdxAmbientOcclusionTask() = delete;
    HdxAmbientOcclusionTask(const HdxAmbientOcclusionTask&) = delete;
    HdxAmbientOcclusionTask(HdxAmbientOcclusionTask&&) = delete;
    HdxAmbientOcclusionTask& operator=(const HdxAmbientOcclusionTask&) = delete;
    HdxAmbientOcclusionTask& operator=(HdxAmbientOcclusionTask&&) = delete;

    HDX_API
    virtual ~HdxAmbientOcclusionTask();

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;

    /// Prepare the tasks resources
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

private:
    HdSt_ImageShaderRenderPassSharedPtr _renderPass;
    HdStRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _renderPassShader;

    HdBufferArrayRangeSharedPtr _kernelBar;
    HdBufferArrayRangeSharedPtr _uniformBar;

    GfMatrix4f _cameraProjection = GfMatrix4f(0.0f);
    SdfPath _cameraId;
    GlfDrawTargetRefPtr _sourceDrawTarget;
    // GlfDrawTargetRefPtr _blurDrawTarget;
    int _aoNumSamples = -1;
    float _aoRadius = -1.0f;
    float _aoAmount = -1.0f;
};

HDX_API
std::ostream& operator<<(
    std::ostream& out, const HdxAmbientOcclusionTaskParams& pv);

HDX_API
bool operator==(
    const HdxAmbientOcclusionTaskParams& lhs,
    const HdxAmbientOcclusionTaskParams& rhs);
HDX_API
bool operator!=(
    const HdxAmbientOcclusionTaskParams& lhs,
    const HdxAmbientOcclusionTaskParams& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_AMBIENTOCCLUSION_TASK_H
