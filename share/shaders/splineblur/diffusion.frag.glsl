#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 out_color;

in vec2 uv;

uniform sampler2D uMainTexture;

void main() {
    ivec2 d = ivec2(1, 0);
    ivec2 xy = ivec2(gl_FragCoord.xy);
    out_color = texelFetch(uMainTexture, xy, 0);
    vec4 a1 = texelFetch(uMainTexture, xy + d.yx, 0);
    vec4 a2 = texelFetch(uMainTexture, xy + d.xy, 0);
    vec4 a3 = texelFetch(uMainTexture, xy - d.yx, 0);
    vec4 a4 = texelFetch(uMainTexture, xy - d.xy, 0);

	float da2;
	vec4 a;
	vec2 v;

	a = a1;
	v = gl_FragCoord.xy - a.xy,
    da2 = dot(v, v);
    if (da2 < out_color.b) {
        out_color = vec4(a.x, a.y, da2, a.a);
    }
    a = a2;
    v = gl_FragCoord.xy - a.xy,
    da2 = dot(v, v);
    if (da2 < out_color.b) {
        out_color = vec4(a.x, a.y, da2, a.a);
    }
    a = a3;
    v = gl_FragCoord.xy - a.xy,
    da2 = dot(v, v);
    if (da2 < out_color.b) {
        out_color = vec4(a.x, a.y, da2, a.a);
    }
    a = a4;
    v = gl_FragCoord.xy - a.xy,
    da2 = dot(v, v);
    if (da2 < out_color.b) {
        out_color = vec4(a.x, a.y, da2, a.a);
    }
}
