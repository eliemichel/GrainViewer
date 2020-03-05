#version 450 core
#include "sys:defines"
layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

/**
 * Shader used for culling in SandRenderer when cullingMechanism is set to
 * PrefixSum. It is split into several steps.
 */

#pragma steps STEP_MARK_CULLING STEP_GROUP STEP_BUILD_COMMAND_BUFFER

struct PointCloundVboEntry {
	vec4 position;
};
struct DrawArraysIndirectCommand  {
	uint  count;
	uint  instanceCount;
	uint  first;
	uint  baseInstance;
};
struct DrawElementsIndirectCommand {
	uint  count;
	uint  instanceCount;
	uint  firstIndex;
	uint  baseVertex;
	uint  baseInstance;
};
struct CommandBuffer {
	DrawElementsIndirectCommand impostorCommand;
	DrawArraysIndirectCommand instanceCommand;
};
struct PrefixSumInfoSsbo {
	uint count[2]; // 0: instanceCount, 1: impostorCount
	// Keep culling flag for last elements, because the prefix sum discards them
	uint isLastPointActive[2]; // 0: isLastPointInstance, 1: isLastPointImpostor
};

layout (std430, binding = 0) buffer prefixSumInfoSsbo {
	PrefixSumInfoSsbo info;
};

uniform uint uPointCount;

// 0: instances
// 1: impostors
// 2: points
uniform uint uType = 0;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/utils.inc.glsl"
#include "include/frustum.inc.glsl"
#include "include/anim.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#ifdef STEP_MARK_CULLING

/**
 * This step put in the renderBit buffer a 1 if the element must be
 * rendered, 0 otherwise. It performs several types of culling depending
 * on the point type beeing impostors or instances
 */

layout(std430, binding = 1) restrict readonly buffer renderTypeSsbo {
	uint renderType[];
};
layout (std430, binding = 2) restrict writeonly buffer impostorElementsSsbo {
	uint renderBit[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;

	// Fake culling, precomputed on CPU before this benchmark
	uint type = renderType[i];
	bool isCulled = type != uType;

	renderBit[i] = isCulled ? 0 : 1;

	// The last value is saved in an extra buffer because after prefix sum this
	// last value (is the only one that) cannot be retrieved
	if (i == uPointCount - 1) {
		info.isLastPointActive[uType] = isCulled ? 0 : 1;
	}
}

#endif // STEP_MARK_CULLING

///////////////////////////////////////////////////////////////////////////////
#ifdef STEP_GROUP

/**
 * Group all activated indices in a contiguous span of the element buffer
 * starting at an offset. The offset is determined by uType: it is instance
 * offset if uType is 0 and impostor offset if it is set to 1.
 * 
 * WARNING: This must be called for instances before impostors because it is
 * also responsible for filling counts info and impostor offset requires to
 * know instance count.
 */

layout (std430, binding = 1) restrict readonly buffer prefixSumSsbo {
	uint prefixSum[];
};
layout (std430, binding = 2) restrict writeonly buffer elementsSsbo {
	uint elements[];
};


void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;

	uint offset = uType == 0 ? 0 : info.count[0];

	uint s = prefixSum[i];
	bool isActive;
	if (i + 1 == uPointCount) {
		info.count[uType] = s;
		isActive = info.isLastPointActive[uType] == 1;
	} else {
		isActive = prefixSum[i+1] != s;
	}

	if (isActive) {
		elements[offset + s] = i;
	}
}

#endif // STEP_GROUP

///////////////////////////////////////////////////////////////////////////////
#ifdef STEP_BUILD_COMMAND_BUFFER

uniform uint uGrainMeshPointCount;

layout (std430, binding = 1) restrict writeonly buffer commandBuffer {
	CommandBuffer cmd;
};

void main() {
	cmd.impostorCommand.count = info.count[1];
	cmd.impostorCommand.instanceCount = 1;
	cmd.impostorCommand.firstIndex = info.count[0];
	cmd.impostorCommand.baseVertex = 0;
	cmd.impostorCommand.baseInstance = 0;
	
	cmd.instanceCommand.count = uGrainMeshPointCount;
	cmd.instanceCommand.instanceCount = info.count[0];
	cmd.instanceCommand.first = 0;
	cmd.instanceCommand.baseInstance = 0;
}

#endif // STEP_BUILD_COMMAND_BUFFER

///////////////////////////////////////////////////////////////////////////////
#if !defined(STEP_MARK_CULLING) && !defined(STEP_GROUP) && !defined(STEP_BUILD_COMMAND_BUFFER)
// Mock step to prevent the compiler from complaining when loading the shader the first time
void main() {}
#endif
