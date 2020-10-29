#version 450 core
#include "sys:defines"

in Geometry {
	vec4 position_cs;
	float radius;
} geo;

layout (location = 0) out vec4 out_color;

#include "../include/uniform/camera.inc.glsl"

#include "../include/utils.inc.glsl"
#include "../include/raytracing.inc.glsl"

void main() {
	/*
	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	vec3 hitPosition;
	if (!intersectRaySphere(hitPosition, ray_cs, geo.position_cs.xyz, geo.radius)) {
		discard; // not recommended
	}
	*/
	out_color = geo.position_cs;
}
