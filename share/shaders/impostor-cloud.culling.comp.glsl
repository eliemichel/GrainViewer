#version 450 core
#include "sys:defines"

// Completely disable culling, more efficiently than setting instanceLimit to zero
#pragma variant ONLY_IMPOSTORS

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

layout (std430, binding = 3) buffer elementsSsbo {
	uint elements[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= nbPoints) return;

	vec3 p = vbo[i].position.xyz;
	vec4 position_cs = viewModelMatrix * vec4(p.xyz, 1.0);

#ifdef ONLY_IMPOSTORS
	elements[i] = i;
	pointers.nextInstanceElement = 0;
	pointers.nextImpostorElement = 0;
#else // ONLY_IMPOSTORS
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
}
