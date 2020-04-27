#version 450 core
#include "sys:defines"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba8, binding = 0) uniform readonly image2D image;
layout (r32i, binding = 1) uniform iimage2D histogram;

void main() {
	ivec3 co3 = ivec3(gl_GlobalInvocationID.xyz);
	ivec2 co = co3.xy;
	ivec3 size = ivec3(imageSize(image), 1);
	if (co3.x >= size.x || co3.y >= size.y || co3.z >= size.z) return;

	vec4 val = imageLoad(image, co);
	imageAtomicAdd(histogram, ivec2(int(val.r * 255), 0), 1);
	imageAtomicAdd(histogram, ivec2(int(val.g * 255), 1), 1);
	imageAtomicAdd(histogram, ivec2(int(val.g * 255), 2), 1);
	imageAtomicAdd(histogram, ivec2(int(val.a * 255), 3), 1);
}
