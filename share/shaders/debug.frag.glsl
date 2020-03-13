#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 color;

in vec2 uv;

uniform sampler2D uMainTexture;

void main() {
	vec4 c = texture(uMainTexture, uv);
	c.b += 0.1;
	color = vec4(c.rgb, 1.0);
}
