#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 color;

in vec2 uv;

uniform sampler2D mainTexture;
uniform float uNear;
uniform float uFar;

void main() {
	float depth = texture(mainTexture, uv).r;
	float ndc = depth * 2.0 - 1.0;
	float linearDepth = (2.0 * uNear * uFar) / (uFar + uNear - ndc * (uFar - uNear));
	linearDepth /= uFar;
	float remappedDepth = depth;
	color = vec4(vec3(linearDepth), 1.0);
}
