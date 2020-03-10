#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 color;

in vec2 uv;

uniform sampler2D mainTexture;

void main() {
	float depth = texture(mainTexture, uv).r;
	float remappedDepth = mix(0.1, 0.9, smoothstep(.99, 1.0, depth));
	color = vec4(vec3(remappedDepth), 1.0);
}
