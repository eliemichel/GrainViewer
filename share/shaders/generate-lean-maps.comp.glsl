#version 450 core
#include "sys:defines"

#pragma variant FLOAT_ELEMENTS // default is UINT_ELEMENTS
#ifdef FLOAT_ELEMENTS
#define ELEMENT_TYPE float
#else // FLOAT_ELEMENTS
#define ELEMENT_TYPE uint
#endif // FLOAT_ELEMENTS

uniform uint uElementCount;
uniform uint uIteration;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform readonly image3D source;
layout (binding = 1) uniform writeonly image3D lean1;
layout (binding = 2) uniform writeonly image3D lean2;

layout (std430, binding = 1) restrict readonly buffer previousElementsSsbo {
	ELEMENT_TYPE previousElements[];
};
layout (std430, binding = 2) restrict writeonly buffer elementsSsbo {
	ELEMENT_TYPE elements[];
};

// TODO: find the actual values
const float sc = 1.0;
const float s = 1.0;

void main() {
	ivec3 co = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 size = imageSize(source);
	if (co.x >= size.x || co.y >= size.y || co.z >= size.z) return;

	vec3 tn = imageLoad(source, co).xyz;
	vec3 N = vec3(2.0 * tn.xy - 1.0, tn.z);

	vec2 B = N.xy / (sc * N.z);
	vec3 M = vec3(B.x * B.x + 1.0 / s, B.y * B.y + 1.0 / s, B.x * B.y);

	imageStore(lean1, co, vec4(tn, M.z * 0.5 + 0.5));
	imageStore(lean2, co, vec4(B * 0.5 + 0.5, M.xy));
}
