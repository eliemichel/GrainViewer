#version 450 core
#include "sys:defines"
#include "sys:settings"

#pragma variant RENDER_TYPE_FORGET RENDER_TYPE_CACHE RENDER_TYPE_PRECOMPUTE
#pragma variant STEP_PRECOMPUTE STEP_RESET STEP_COUNT STEP_OFFSET STEP_WRITE

layout (local_size_x = LOCAL_SIZE_X, local_size_y = 1, local_size_z = 1) in;

struct PointCloundVboEntry {
	vec4 position;
};

struct Counter {
	uint count;
	uint offset;
};

/**
 * Shader used for culling in SandRenderer when cullingMechanism is set to
 * PrefixSum. It is split into several steps.
 */

uniform uint uPointCount;
uniform uint uRenderModelCount = 2;
uniform uint uFrameCount;
uniform float uFps = 25.0;
uniform float uTime;

layout(std430, binding = 0) restrict buffer countersSsbo {
	Counter counters[];
};
layout (std430, binding = 2) restrict writeonly buffer elementBufferSsbo {
	uint elementBuffer[];
};

/**
 * getRenderType(): Get the index of the model to use for a given element.
 * Several options to investigate:
 *   a. cache this before hand in a renderType[] ssbo
        -- RENDER_TYPE_PRECOMPUTED variant
 *   b. compute it directly here, but then compute it twice (steps #1 and #3)
        -- RENDER_TYPE_FORGET variant
 *   c. A trade-off would consist in computing it during step #1 and caching
 *      it for step #3 -- RENDER_TYPE_CACHED variant
 * The best choice depends on whether the limiting factor is memory bandwidth
 * or scalar units.
 */
///////////////////////////////////////////////////////////////////////////////
#if (defined(RENDER_TYPE_PRECOMPUTED) && !defined(STEP_PRECOMPUTE)) || (defined(RENDER_TYPE_CACHE) && !defined(STEP_COUNT))

layout(std430, binding = 1) restrict readonly buffer renderTypeSsbo {
	uint renderType[];
};

uint getRenderType(uint element) {
	return renderType[element];
}

///////////////////////////////////////////////////////////////////////////////
#else // RENDER_TYPE_PRECOMPUTED

uniform sampler2D uOcclusionMap;
uniform float uOuterOverInnerRadius;
uniform float uInnerRadius;
uniform float uOuterRadius;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "../include/uniform/camera.inc.glsl"

#include "../include/anim.inc.glsl"

#include "discriminate.inc.glsl"

#if defined(RENDER_TYPE_CACHE) || defined(STEP_PRECOMPUTE)
layout(std430, binding = 1) restrict writeonly buffer renderTypeSsbo {
	uint renderType[];
};
#endif // RENDER_TYPE_CACHE

layout(std430, binding = 3) restrict readonly buffer pointSsbo {
	PointCloundVboEntry point[];
};

uint getRenderType(uint element) {
	uint totalPointCount = uPointCount * uFrameCount;
	uint pointId = AnimatedPointId(element, uFrameCount, totalPointCount, uTime, uFps);
	vec3 position = point[pointId].position.xyz;
	uint model = discriminate(position, uOuterRadius, uInnerRadius, uOuterOverInnerRadius, uOcclusionMap);
#ifdef RENDER_TYPE_CACHE
	renderType[element] = model;
#endif // RENDER_TYPE_CACHE
	return model;
}

///////////////////////////////////////////////////////////////////////////////
#endif // RENDER_TYPE_PRECOMPUTED

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;
	uint type, beforeIncrement;

///////////////////////////////////////////////////////////////////////////////
#if defined(STEP_PRECOMPUTE)
// Reset counters (shader invoked only once at this step)
	renderType[i] = getRenderType(i);

///////////////////////////////////////////////////////////////////////////////
#elif defined(STEP_RESET)
// Reset counters (shader invoked only once at this step)
	for (type = 0 ; type < uRenderModelCount ; ++type) {
		counters[type].count = 0;
	}

///////////////////////////////////////////////////////////////////////////////
#elif defined(STEP_COUNT)
// Count elements of each render type
	type = getRenderType(i);
	atomicAdd(counters[type].count, 1);

///////////////////////////////////////////////////////////////////////////////
#elif defined(STEP_OFFSET)
// Compute offsets (shader invoked only once at this step)
	counters[0].offset = 0;
	for (type = 0 ; type < uRenderModelCount - 1 ; ++type) {
		counters[type + 1].offset = counters[type].offset + counters[type].count;
		counters[type].count = 0;
	}
	counters[uRenderModelCount - 1].count = 0;

///////////////////////////////////////////////////////////////////////////////
#elif defined(STEP_WRITE)
// Finally write to element buffer
	type = getRenderType(i);
	beforeIncrement = atomicAdd(counters[type].count, 1);
	elementBuffer[counters[type].offset + beforeIncrement] = i;

#endif // STEP	

}
