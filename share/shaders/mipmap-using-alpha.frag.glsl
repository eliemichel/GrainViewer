#version 450 core
#include "sys:defines"

in vec2 vertUv;

uniform sampler2DArray uPreviousLevel; // previous level, where alpha is valid
uniform sampler2DArray uAlphaLevel; // current level, alpha texture
uniform int uLevel;
uniform int uLayer;
uniform bool isPremultiplied = false;

layout (location = 0) out vec4 out_color;

void main() {
    ivec2 d = ivec2(-1, 0);
    ivec3 xyz = ivec3(ivec2(2 * gl_FragCoord.xy), uLayer);

    vec4 c00 = texelFetch(uPreviousLevel, xyz + d.yyy, 0);
    vec4 c10 = texelFetch(uPreviousLevel, xyz + d.xyy, 0);
    vec4 c01 = texelFetch(uPreviousLevel, xyz + d.yxy, 0);
    vec4 c11 = texelFetch(uPreviousLevel, xyz + d.xxy, 0);

    float a00 = c00.a;
	float a10 = c10.a;
	float a01 = c01.a;
	float a11 = c11.a;

    if (uLevel == 0) {
	    a00 = texelFetch(uAlphaLevel, xyz + d.yyy, 0).a;
		a10 = texelFetch(uAlphaLevel, xyz + d.xyy, 0).a;
		a01 = texelFetch(uAlphaLevel, xyz + d.yxy, 0).a;
		a11 = texelFetch(uAlphaLevel, xyz + d.xxy, 0).a;
	}

	float sum = a00 + a10 + a01 + a11;
	//if (isPremultiplied) {
	//} else {
	    out_color.rgb = c00.rgb * a00 + c10.rgb * a10 + c01.rgb * a01 + c11.rgb * a11;
	    out_color.rgb /= sum;
	    out_color.a = sum  * 0.25;
	//	out_color.rgb = c00.rgb + c10.rgb + c01.rgb + c11.rgb;
	//}

	//out_color = (c00 + c01 + c10 + c11) * 0.25;
	//out_color = vec4(1.0, 0.5, 0.0, 1.0);
}
