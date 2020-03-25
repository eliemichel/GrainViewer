#version 450 core
#include "sys:defines"

#pragma variant STAGE_EPSILON_ZBUFFER

layout (location = 0) in vec4 position;

out VertexData {
	vec3 position_ws;
#ifdef PROCEDURAL_BASECOLOR3
	vec3 originalPosition_ws;
#endif // PROCEDURAL_BASECOLOR3
	float radius;
	uint vertexId;
} outData;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;

uniform float uRadius = 0.005;
uniform float uTime;

void main() {
	vec3 p = position.xyz;

#ifdef PROCEDURAL_ANIM0
#ifdef PROCEDURAL_BASECOLOR3
	outData.originalPosition_ws = (modelMatrix * vec4(p, 1.0)).xyz;
#endif // PROCEDURAL_BASECOLOR3
	float t = uTime * 0.;
	p *= 1 + sin(t * 2.0 + p.y * 2.0) * 0.1 * sin(atan(p.x, p.z) * 10.0);
#endif // PROCEDURAL_ANIM0

	outData.radius = uRadius;
	outData.position_ws = (modelMatrix * vec4(p, 1.0)).xyz;
	outData.vertexId = gl_VertexID;
}

