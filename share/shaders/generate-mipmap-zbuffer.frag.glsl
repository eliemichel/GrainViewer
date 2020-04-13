#version 450 core
#include "sys:defines"

in vec2 uv;

uniform sampler2D previousLevel;

void main() {
    ivec2 d = ivec2(-1, 0);
    ivec2 xy = ivec2(2 * gl_FragCoord.xy);
    float z00 = texelFetch(previousLevel, xy + d.yy, 0).r;
    float z10 = texelFetch(previousLevel, xy + d.xy, 0).r;
    float z01 = texelFetch(previousLevel, xy + d.yx, 0).r;
    float z11 = texelFetch(previousLevel, xy + d.xx, 0).r;
    gl_FragDepth = max(max(z00, z01), max(z10, z11));
}
