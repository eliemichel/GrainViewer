#version 450 core
#include "sys:defines"

layout (location = 0) in vec2 iUv;

out vec2 uv;

uniform vec2 uPosition;
uniform vec2 uSize;
uniform vec2 uResolution;

vec2 mapToClipSpace(vec2 pixelCo) {
	return (pixelCo / uResolution * 2. - 1.) * vec2(1,-1);
}

void main() {
	gl_Position = vec4(mapToClipSpace(uPosition + iUv * uSize), 0.0, 1.0);
	uv = iUv;
}
