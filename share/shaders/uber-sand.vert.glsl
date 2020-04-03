#version 450 core
#include "sys:defines"

#pragma varopt PASS_BLIT_TO_MAIN_FBO

layout (location = 0) in vec4 position;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) buffer pointElementsSsbo {
    uint pointElements[];
};

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

// just a regular post effect
out vec2 vertUv;
void main() {
	gl_Position = vec4(position.xyz, 1.0);
	vertUv = position.xy * .5 + .5;
}

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

out VertexData {
	vec3 position_ws;
	vec3 originalPosition_ws; // for procedural color
	float radius;
	uint vertexId;
} outData;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;

uniform float uGrainRadius = 0.005;
uniform float uTime;

uniform bool uUsePointElements = true;

void main() {
    uint pointId =
    	uUsePointElements
    	? pointElements[gl_VertexID]
    	: gl_VertexID;

	vec3 p = uUsePointElements ? pointVertexAttributes[pointId].position.xyz : position.xyz;

#ifdef PROCEDURAL_ANIM0
	float t = uTime * 0.;
	p *= 1 + sin(t * 2.0 + p.y * 2.0) * 0.1 * sin(atan(p.x, p.z) * 10.0);
#endif // PROCEDURAL_ANIM0

	outData.radius = uGrainRadius;
	outData.position_ws = (modelMatrix * vec4(p, 1.0)).xyz;
	outData.originalPosition_ws = (modelMatrix * vec4(p, 1.0)).xyz;
	outData.vertexId = pointId;
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS_BLIT_TO_MAIN_FBO
