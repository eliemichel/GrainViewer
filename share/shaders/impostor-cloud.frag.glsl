#version 450 core
#include "sys:defines"

flat in int id;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = vec4(float((id/255/255) % 255) / 255, float((id/255) % 255) / 255, float(id % 255) / 255, 1.0);
}
