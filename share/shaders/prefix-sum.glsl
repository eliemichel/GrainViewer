#version 450 core
#include "sys:defines"

uniform float uTime;
uniform uint uPointCount;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) buffer elementsSsbo {
	uint elements[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uPointCount) return;
	//elements[i];
}
