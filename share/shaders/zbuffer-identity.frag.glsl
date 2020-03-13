#version 450 core
#include "sys:defines"

uniform sampler2D uMainTexture;

void main() {
	ivec2 d = ivec2(1, 0);
    ivec2 xy = ivec2(2 * gl_FragCoord.xy);
    float z00 = texelFetch(uMainTexture, xy + d.yy, 0).r;
    float z10 = texelFetch(uMainTexture, xy + d.xy, 0).r;
    float z01 = texelFetch(uMainTexture, xy + d.yx, 0).r;
    float z11 = texelFetch(uMainTexture, xy + d.xx, 0).r;
    gl_FragDepth = max(max(z00, z01), max(z10, z11));
    //gl_FragDepth = texelFetch(uMainTexture, ivec2(gl_FragCoord.xy), 0).r;
}
