#version 450 core
#include "sys:defines"

in vec4 position_cs;
in float radius;

layout (location = 0) out vec4 out_color;

#include "../include/uniform/camera.inc.glsl"

#include "../include/utils.inc.glsl"
#include "../include/raytracing.inc.glsl"

void main() {
	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	vec3 hitPosition;
	if (!intersectRaySphere(hitPosition, ray_cs, position_cs.xyz, radius)) {
		//discard; // not recommended
	}
	out_color = position_cs;
}
