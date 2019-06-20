#include "pxr/imaging/hdx/ambientOcclusionTask.h"

PXR_NAMESPACE_OPEN_SCOPE

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

}

void HdxAmbientOcclusionTask::Execute(HdTaskContext* ctx)
{

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
