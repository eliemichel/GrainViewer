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

uniform uint uPointCountPerFrame;
uniform uint uPointCount;

// 0: instances
// 1: impostors
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

uniform float uInstanceLimit = 1.05;
uniform bool uEnableOcclusionCulling = true;
uniform bool uEnableDistanceCulling = true;
uniform bool uEnableFrustumCulling = true;

uniform sampler2D occlusionMap;
uniform float uOuterOverInnerRadius;
uniform float uInnerRadius;
uniform float uOuterRadius;

uniform uint uFrameCount;
uniform float uFps = 25.0;
uniform float uTime;

layout(std430, binding = 1) restrict readonly buffer pointSsbo {
	PointCloundVboEntry point[];
};
layout (std430, binding = 2) restrict writeonly buffer impostorElementsSsbo {
	uint renderBit[];
};

bool culling(vec3 position, float near, float far, float outerRadius, float innerRadius, float outerOverInnerRadius)
{
	vec4 position_cs = viewModelMatrix * vec4(position, 1.0);

	/////////////////////////////////////////
	// Distance culling
	if (uEnableDistanceCulling) {
		float l = length(position_cs);
		if (l >= near && l < far) {
			return true;
		}
	}

	/////////////////////////////////////////
	// Frustum culling
	// unexplained multiplication factor...
	float fac = 4.0;
	if (uEnableFrustumCulling && SphereFrustumCulling(projectionMatrix, position_cs.xyz, fac*outerRadius)) {
		return true;
	}

	/////////////////////////////////////////
	// Occlusion culling
	if (uEnableOcclusionCulling) {
		vec4 position_ps = projectionMatrix * vec4(position_cs.xyz, 1.0);
		position_ps = position_ps / position_ps.w * 0.5 + 0.5;
		position_ps.xy = clamp(position_ps.xy, vec2(0.0), vec2(1.0));
		vec3 otherGrain_cs = textureLod(occlusionMap, position_ps.xy, 0).xyz;
		vec3 closestCone_cs = otherGrain_cs * outerOverInnerRadius;
		float cosAlpha = dot(normalize(closestCone_cs), normalize(position_cs.xyz - closestCone_cs));
		if (cosAlpha >= 0 && closestCone_cs != vec3(0.0)) {
			float sinBeta = innerRadius / length(otherGrain_cs);
			float sin2Alpha = 1. - cosAlpha * cosAlpha;
			float sin2Beta = sinBeta * sinBeta;
			if (sin2Alpha < sin2Beta) {
				return true;
			}
		}
	}

	return false;
}

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCountPerFrame) return;

	uint pointId = AnimatedPointId(i, uFrameCount, uPointCount, uTime, uFps);
	vec3 position = point[pointId].position.xyz;

	float near = getNearDistance(projectionMatrix);
	float far = getFarDistance(projectionMatrix);

	if (uType == 0) {
		near = uInstanceLimit;
	} else if (uType == 1) {
		far = uInstanceLimit;
	}

	bool isCulled = culling(position, near, far, uOuterRadius, uInnerRadius, uOuterOverInnerRadius);

	renderBit[i] = isCulled ? 0 : 1;

	// The last value is saved in an extra buffer because after prefix sum this
	// last value (is the only one that) cannot be retrieved
	if (i == uPointCountPerFrame - 1) {
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
	if (i >= uPointCountPerFrame) return;

	uint offset = uType == 0 ? 0 : info.count[0];

	uint s = prefixSum[i];
	bool isActive;
	if (i + 1 == uPointCountPerFrame) {
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
