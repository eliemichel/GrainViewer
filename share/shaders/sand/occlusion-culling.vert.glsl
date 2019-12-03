#version 450 core
#include "sys:defines"

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer points {
	PointCloundVboEntry vbo[];
};

out vec4 position_cs;
out float radius;

uniform float uOuterOverInnerRadius = 1.0 / 0.5;
uniform float uInnerRadius;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "../include/uniform/camera.inc.glsl"

#include "../include/utils.inc.glsl"
#include "../include/sprite.inc.glsl"

void main() {
	vec4 position_ls = vec4(vbo[gl_VertexID].position.xyz, 1.0);
	position_cs = viewModelMatrix * position_ls;
	radius= uInnerRadius;
	gl_Position = projectionMatrix * position_cs;
	gl_PointSize = SpriteSize(radius, gl_Position);
}

