#include "BehaviorRegistry.h"
#include "Behavior.h"
#include "RuntimeObject.h"

#include "Behavior/MeshDataBehavior.h"
#include "Behavior/MeshRenderer.h"
#include "Behavior/ImpostorCloudRenderer.h"
#include "Behavior/SandRenderer.h"
#include "Behavior/TransformBehavior.h"
#include "Behavior/TestPrefixSumRenderer.h"
#include "Behavior/LightGizmo.h"
#include "Behavior/PointCloudDataBehavior.h"
#include "Behavior/PointCloudSplitter.h"
#include "Behavior/Sand6Data.h"
#include "Behavior/FarSandRenderer.h"
#include "Behavior/UberSandRenderer.h"
#include "Behavior/GltfDataBehavior.h"
#include "Behavior/InstanceSandRenderer.h"
#include "Behavior/ImpostorSandRenderer.h"
#include "Behavior/SandBehavior.h"

void BehaviorRegistry::addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type)
{
#define handleType(T) if (type == TypeName<T>().Get()) { b = IBehaviorHolder::addBehavior<T>(obj); }
	handleType(MeshDataBehavior);
	handleType(MeshRenderer);
	handleType(ImpostorCloudRenderer);
	handleType(SandRenderer);
	handleType(TransformBehavior);
	handleType(TestPrefixSumRenderer);
	handleType(LightGizmo);
	handleType(PointCloudDataBehavior);
	handleType(PointCloudSplitter);
	handleType(Sand6Data);
	handleType(FarSandRenderer);
	handleType(UberSandRenderer);
	handleType(GltfDataBehavior);
	handleType(InstanceSandRenderer);
	handleType(ImpostorSandRenderer);
	handleType(SandBehavior);
#undef handleType
}
