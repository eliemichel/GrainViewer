#include "BehaviorRegistry.h"
#include "Behavior.h"
#include "RuntimeObject.h"

#ifdef WITH_GPL
#include "Behavior/Sand6Data.h"
#endif // WITH_GPL
#include "Behavior/MeshDataBehavior.h"
#include "Behavior/MeshRenderer.h"
#include "Behavior/ImpostorCloudRenderer.h"
#include "Behavior/SandRenderer.h"
#include "Behavior/TransformBehavior.h"
#include "Behavior/TestPrefixSumRenderer.h"
#include "Behavior/LightGizmo.h"
#include "Behavior/PointCloudDataBehavior.h"
#include "Behavior/PointCloudSplitter.h"
#include "Behavior/FarSandRenderer.h"
#include "Behavior/UberSandRenderer.h"
#include "Behavior/GltfDataBehavior.h"
#include "Behavior/InstanceSandRenderer.h"
#include "Behavior/ImpostorSandRenderer.h"
#include "Behavior/SandBehavior.h"
#include "Behavior/QuadMeshData.h"

void BehaviorRegistry::addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type)
{
#define handleType(T) if (type == TypeName<T>().Get()) { b = IBehaviorHolder::addBehavior<T>(obj); }
#ifdef WITH_GPL
	handleType(Sand6Data);
#endif // WITH_GPL
	handleType(MeshDataBehavior);
	handleType(MeshRenderer);
	handleType(ImpostorCloudRenderer);
	handleType(SandRenderer);
	handleType(TransformBehavior);
	handleType(TestPrefixSumRenderer);
	handleType(LightGizmo);
	handleType(PointCloudDataBehavior);
	handleType(PointCloudSplitter);
	handleType(FarSandRenderer);
	handleType(UberSandRenderer);
	handleType(GltfDataBehavior);
	handleType(InstanceSandRenderer);
	handleType(ImpostorSandRenderer);
	handleType(SandBehavior);
	handleType(QuadMeshData);
#undef handleType
}

std::weak_ptr<IPointCloudData> BehaviorRegistry::getPointCloudDataComponent(Behavior& behavior, PointCloudSplitter::RenderModel preferedModel) {
	std::weak_ptr<IPointCloudData> pointData;
	if (preferedModel != PointCloudSplitter::RenderModel::None) {
		if (auto splitter = behavior.getComponent<PointCloudSplitter>().lock()) {
			pointData = splitter->subPointCloud(preferedModel);
		}
	}
	if (pointData.expired()) pointData = behavior.getComponent<PointCloudDataBehavior>();
#ifdef WITH_GPL
	if (pointData.expired()) pointData = behavior.getComponent<Sand6Data>();
#endif // WITH_GPL
	if (pointData.expired()) {
		WARN_LOG
			<< "Could not find point data "
			<< "(ensure that there is one of PointCloudSplitter, "
			<< "PointCloudDataBehavior or Sand6Data attached to the same"
			<< "object)";
	}
	return pointData;
}
