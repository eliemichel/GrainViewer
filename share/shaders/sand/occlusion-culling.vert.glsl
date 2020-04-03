#version 450 core
#include "sys:defines"

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) restrict readonly buffer points {
	PointCloundVboEntry vbo[];
};

out vec4 position_cs;
out float radius;

uniform float uOuterOverInnerRadius = 1.0 / 0.5;
uniform float uGrainRadius;
uniform float uGrainInnerRadiusRatio;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "../include/uniform/camera.inc.glsl"

#include "../include/utils.inc.glsl"
#include "../include/sprite.inc.glsl"

void main() {
	vec4 position_ls = vec4(vbo[gl_VertexID].position.xyz, 1.0);
	position_cs = viewModelMatrix * position_ls;
	radius = uGrainRadius * uGrainInnerRadiusRatio; // inner radius
	gl_Position = projectionMatrix * position_cs;
	// The *.15 has no explaination, but it empirically increases occlusion culling
	gl_PointSize = SpriteSize(radius, gl_Position) * 0.25;
}

