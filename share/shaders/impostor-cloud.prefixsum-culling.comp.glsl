#version 450 core
#include "sys:defines"
layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

/**
 * Shader used for culling in SandRenderer when cullingMechanism is set to
 * PrefixSum. It is split into several steps.
 */

#pragma steps STEP_MARK_IMPOSTORS STEP_MARK_INSTANCES STEP_GROUP STEP_BUILD_COMMAND_BUFFER

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
uniform float uInstanceLimit = 1.05;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#ifdef STEP_MARK_IMPOSTORS

/**
 * This step put in the renderAsImpostor buffer a 1 if the element must be
 * rendered using an impostor, 0 otherwise.
 */

layout(std430, binding = 1) restrict readonly buffer pointSsbo {
	PointCloundVboEntry point[];
};
layout (std430, binding = 2) restrict writeonly buffer impostorElementsSsbo {
	uint renderAsImpostor[];
};

uniform sampler2D depthMap;
uniform float uInnerOverOuterRadius = 1.0 / 1.0;

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;

	float radius_ps = 0.05; // TODO

	vec3 p = point[i].position.xyz;
	vec4 position_cs = viewModelMatrix * vec4(p.xyz, 1.0);
	vec4 occlusion_position_cs = vec4(position_cs.xyz * uInnerOverOuterRadius, 1.0);
	vec4 occlusion_position_ps = projectionMatrix * occlusion_position_cs;
	occlusion_position_ps = occlusion_position_ps / occlusion_position_ps.w;

	bool frustumCulled = abs(occlusion_position_ps.x) >= 1 + radius_ps || abs(occlusion_position_ps.y) >= 1 + radius_ps || occlusion_position_ps.z < 0;

	occlusion_position_ps = occlusion_position_ps * 0.5 + 0.5;

	//bool occlusionCulled = occlusion_position_ps.z > textureLod(depthMap, occlusion_position_ps.xy, 0).r / uInnerOverOuterRadius;
	bool occlusionCulled = occlusion_position_ps.z > textureLod(depthMap, occlusion_position_ps.xy, 0).r;
	bool distanceCulled = length(position_cs) < uInstanceLimit;

	uint isImpostor = occlusionCulled || distanceCulled || frustumCulled ? 0 : 1;
	//isImpostor = occlusionCulled ? 1 : 0;

	renderAsImpostor[i] = isImpostor;

	if (i == uPointCount - 1) {
		info.isLastPointActive[1] = isImpostor;
	}
}

#endif // STEP_MARK_IMPOSTORS

///////////////////////////////////////////////////////////////////////////////
#ifdef STEP_MARK_INSTANCES

/**
 * This step put in the renderAsInstance buffer a 1 if the element must be
 * rendered using an impostor, 0 otherwise.
 */

layout(std430, binding = 1) restrict readonly buffer pointSsbo {
	PointCloundVboEntry point[];
};
layout (std430, binding = 2) restrict writeonly buffer instanceElementsSsbo {
	uint renderAsInstance[];
};


void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;

	vec3 p = point[i].position.xyz;
	vec4 position_cs = viewModelMatrix * vec4(p.xyz, 1.0);

	uint isInstance = 0;
	if (length(position_cs) < uInstanceLimit) {
		isInstance = 1;
	}

	renderAsInstance[i] = isInstance;

	if (i == uPointCount - 1) {
		info.isLastPointActive[0] = isInstance;
	}
}

#endif // STEP_MARK_INSTANCES

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

uniform uint uType = 0;

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
	if (i == uPointCount - 1) {
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
