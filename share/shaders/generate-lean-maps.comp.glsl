#version 450 core
#include "sys:defines"

#if defined(USING_IMAGE_2D)
#define IMAGE_TYPE image2D
#define NB_DIM 2
#elif defined(USING_IMAGE_2D_ARRAY)
#define IMAGE_TYPE image2DArray
#define NB_DIM 3
#elif defined(USING_IMAGE_3D)
#define IMAGE_TYPE image3D
#define NB_DIM 3
#endif // USING_IMAGE_*

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout  (rgba8, binding = 0) uniform readonly IMAGE_TYPE source;
layout (rgba16f, binding = 1) uniform writeonly IMAGE_TYPE lean1;
layout (rgba16f, binding = 2) uniform writeonly IMAGE_TYPE lean2;

// TODO: find the actual values
const float sc = 1.0;
const float s = 4.0; // Blinn-Phong exponent

void main() {
#if NB_DIM == 2
	ivec3 co3 = ivec3(gl_GlobalInvocationID.xyz);
	ivec2 co = co3.xy;
	ivec3 size = ivec3(imageSize(source), 1);
#else // NB_DIM
	ivec3 co3 = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 co = co3;
	ivec3 size = imageSize(source);
#endif // NB_DIM

	if (co3.x >= size.x || co3.y >= size.y || co3.z >= size.z) return;

	vec3 tn = imageLoad(source, co).xyz;
	vec3 N = vec3(2.0 * tn.xy - 1.0, tn.z);

	vec2 B = N.xy / (sc * N.z);
	vec3 M = vec3(B.x * B.x + 1.0 / s, B.y * B.y + 1.0 / s, B.x * B.y);

	imageStore(lean1, co, vec4(tn, M.z * 0.5 + 0.5));
	imageStore(lean2, co, vec4(B * 0.5 + 0.5, M.xy));
}
