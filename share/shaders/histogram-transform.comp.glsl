#version 450 core
#include "sys:defines"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba8, binding = 0) uniform readonly image2D source;
layout (rgba8, binding = 1) uniform writeonly image2D destination;
layout (r32i, binding = 2) uniform readonly iimage2D cdf;

uniform int uStep = 0;

void main() {
	ivec2 co = ivec3(gl_GlobalInvocationID.xyz).xy;
	ivec3 size = ivec3(imageSize(destination), 1);
	if (co.x >= size.x || co.y >= size.y) return;

	vec4 color = imageLoad(source, co);

	ivec4 bins = ivec4(
		imageLoad(cdf, ivec2(color.r * 255, 0)).r,
		imageLoad(cdf, ivec2(color.g * 255, 1)).r,
		imageLoad(cdf, ivec2(color.b * 255, 2)).r,
		imageLoad(cdf, ivec2(color.a * 255, 3)).r
	);

	int total = imageLoad(cdf, ivec2(255, 0)).r;

	color = vec4(bins) / float(total);

	imageStore(destination, co, color);
}
