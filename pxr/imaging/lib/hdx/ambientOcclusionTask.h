#ifndef HDX_AMBIENTOCCLUSION_TASK_H
#define HDX_AMBIENTOCCLUSION_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

struct HdxAmbientOcclusionTaskParams {
    bool enable = false;
};

typedef boost::shared_ptr<class HdRenderPassState>
    HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass>
    HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassShader>
    HdStRenderPassShaderSharedPtr;

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
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _renderPassShader;

    int _numSamples = -1;
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
