#version 450 core
#include "sys:defines"

in vec2 uv;
layout (location = 0) out vec4 color;

uniform isampler2D uHistogram;
uniform float uNormalization = 1.0;

void main() {
	vec4 hist = vec4(
		texelFetch(uHistogram, ivec2(uv.x * 255, 0), 0).x,
		texelFetch(uHistogram, ivec2(uv.x * 255, 1), 0).x,
		texelFetch(uHistogram, ivec2(uv.x * 255, 2), 0).x,
		texelFetch(uHistogram, ivec2(uv.x * 255, 3), 0).x
	) * uNormalization;
    color = step(vec4(1. - uv.y), hist);
    //color = vec4(hist.rgb, 1.0);
}
