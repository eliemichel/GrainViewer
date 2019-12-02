#version 450 core
#include "sys:defines"

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer points {
	PointCloundVboEntry vbo[];
};

uniform float uOuterOverInnerRadius = 1.0 / 0.5;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "../include/uniform/camera.inc.glsl"

void main() {
	vec4 position_ls = vec4(vbo[gl_VertexID].position.xyz, 1.0);
	vec4 position_cs = viewModelMatrix * position_ls;

	// Occlusion Correction
	//position_cs.z *= uOuterOverInnerRadius;

	gl_Position = projectionMatrix * position_cs;
}

