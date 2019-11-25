#version 450 core
#include "sys:defines"

flat in int vId;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = vec4(1.0);
}
