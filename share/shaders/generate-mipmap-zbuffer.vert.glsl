#version 450 core
#include "sys:defines"

layout (location = 0) in vec3 position;

out vec2 uv;

void main() {
	gl_Position = vec4(position, 1.0);
	uv = position.xy * .5 + .5;
}

