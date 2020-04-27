#version 450 core
#include "sys:defines"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba8, binding = 0) uniform readonly image2D image;
layout (r32i, binding = 1) uniform iimage2D histogram;

uniform int uStep = 0;

void main() {
	if (uStep == 0) {
		// init to 0
		ivec2 co = ivec3(gl_GlobalInvocationID.xyz).xy;
		ivec3 size = ivec3(imageSize(histogram), 1);
		if (co.x >= size.x || co.y >= size.y) return;

		imageStore(histogram, co, ivec4(0));
	}
	else if (uStep == 1) {
		// accumulate	
		ivec2 co = ivec3(gl_GlobalInvocationID.xyz).xy;
		ivec3 size = ivec3(imageSize(image), 1);
		if (co.x >= size.x || co.y >= size.y) return;

		vec4 val = imageLoad(image, co);
		imageAtomicAdd(histogram, ivec2(int(val.r * 255), 0), 1);
		imageAtomicAdd(histogram, ivec2(int(val.g * 255), 1), 1);
		imageAtomicAdd(histogram, ivec2(int(val.b * 255), 2), 1);
		imageAtomicAdd(histogram, ivec2(int(val.a * 255), 3), 1);
	}
}
