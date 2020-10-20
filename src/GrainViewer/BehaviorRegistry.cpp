/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

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
	if (pointData.expired()) {
		WARN_LOG
			<< "Could not find point data "
			<< "(ensure that there is one of PointCloudSplitter, "
			<< "PointCloudDataBehavior or Sand6Data attached to the same"
			<< "object)";
	}
	return pointData;
}
