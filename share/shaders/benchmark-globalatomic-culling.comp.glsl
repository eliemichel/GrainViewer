#version 450 core
#include "sys:defines"
layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

/**
 * Shader used for culling in SandRenderer when cullingMechanism is set to
 * PrefixSum. It is split into several steps.
 */

uniform uint uPointCount;
uniform uint uRenderModelCount = 2;
uniform uint uStep = 0; // 0: reset counters, 1: count only, 2: compute offsets, 3: write EBO

struct Counter {
	uint count;
	uint offset;
};

layout(std430, binding = 0) restrict buffer countersSsbo {
	Counter counters[];
};
layout(std430, binding = 1) restrict readonly buffer renderTypeSsbo {
	uint renderType[];
};
layout (std430, binding = 2) restrict writeonly buffer elementBufferSsbo {
	uint elementBuffer[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;
	uint type, beforeIncrement;

	switch (uStep) {
	case 0: // Reset counters (shader invoked only once at this step)
		for (type = 0 ; type < uRenderModelCount ; ++type) {
			counters[type].count = 0;
		}
		break;

	case 1: // Count elements of each render type
		type = renderType[i];
		atomicAdd(counters[type].count, 1);
		break;

	case 2: // Compute offsets (shader invoked only once at this step)
		counters[0].offset = 0;
		for (type = 0 ; type < uRenderModelCount - 1 ; ++type) {
			counters[type + 1].offset = counters[type].offset + counters[type].count;
			counters[type].count = 0;
		}
		counters[uRenderModelCount - 1].count = 0;
		break;

	case 3: // Finally write to element buffer
		type = renderType[i];
		beforeIncrement = atomicAdd(counters[type].count, 1);
		elementBuffer[counters[type].offset + beforeIncrement] = i;
		break;
	}
}
