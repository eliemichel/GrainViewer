#version 450 core
#include "sys:defines"

// Completely disable culling, more efficiently than setting instanceLimit to zero
#pragma variant ONLY_IMPOSTORS
// Instead of sorting elements with costy atomic operations, use two elements
// buffer and leverage on RESTART_PRIMITIVE to have the hardawre cull some
// elements on draw call. This is faster, at the cost of more memory usage.
// WARNING: Do not set this manually, use the doubleElementBuffer option of the
// SandRenderer, because enabling this implies changes on CPU side as well.
#pragma hidden_variant DOUBLE_ELEMENT_BUFFER

#define RESTART_PRIMITIVE (0xFFFFFFF0)

uniform uint nbPoints;
uniform float instanceLimit = 1.05;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer pointsSsbo {
	PointCloundVboEntry vbo[];
};

struct Pointers {
	int nextInstanceElement;
	int nextImpostorElement;
	int _pad0[2];
};
layout (std430, binding = 2) buffer pointersSsbo {
	Pointers pointers;
};

#ifdef DOUBLE_ELEMENT_BUFFER
layout (std430, binding = 3) restrict writeonly buffer impostorElementsSsbo {
	uint impostorElements[];
};
layout (std430, binding = 4) restrict writeonly buffer instanceElementsSsbo {
	uint instanceElements[];
};
#else // DOUBLE_ELEMENT_BUFFER
layout (std430, binding = 3) restrict writeonly buffer elementsSsbo {
	uint elements[];
};
#endif // DOUBLE_ELEMENT_BUFFER

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= nbPoints) return;

	vec3 p = vbo[i].position.xyz;
	vec4 position_cs = viewModelMatrix * vec4(p.xyz, 1.0);

	// 1. Frustum culling
	if (position_cs.z < 0 || 0 == 0) {
		//return;
	}

	// 2. Model dispatch (impostor or instance)

#ifdef DOUBLE_ELEMENT_BUFFER
	if (length(position_cs) < instanceLimit) {
		// will be drawn as an instance
		instanceElements[i] = i;
		impostorElements[i] = RESTART_PRIMITIVE;
	} else {
		// will be drawn as an impostor
		int nid = atomicAdd(pointers.nextImpostorElement, -1);
		instanceElements[i] = RESTART_PRIMITIVE;
		impostorElements[i] = i;
	}
#else // DOUBLE_ELEMENT_BUFFER
#ifdef ONLY_IMPOSTORS
	elements[i] = i;
	pointers.nextInstanceElement = 0;
	pointers.nextImpostorElement = -1;
#else // ONLY_IMPOSTORS
	// atomic sum version, costy (TODO: try smarter prefix sum)
	if (length(position_cs) < instanceLimit) {
		// will be drawn as an instance
		int nid = atomicAdd(pointers.nextInstanceElement, 1);
		elements[nid] = i;
	} else {
		// will be drawn as an impostor
		int nid = atomicAdd(pointers.nextImpostorElement, -1);
		elements[nid] = i;
	}
#endif // ONLY_IMPOSTORS
#endif // DOUBLE_ELEMENT_BUFFER
}
