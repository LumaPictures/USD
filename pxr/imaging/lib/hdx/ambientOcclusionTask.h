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

class HdxAmbientOcclusionTask : public HdTask {
public:
    HDX_API
    HdxAmbientOcclusionTask(HdSceneDelegate* delegate, const SdfPath& id);

    HDX_API
    virtual ~HdxAmbientOcclusionTask();

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;
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
