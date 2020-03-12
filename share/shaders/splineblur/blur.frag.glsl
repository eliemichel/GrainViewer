#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 out_color;

in vec2 uv;

uniform sampler2D uMainTexture;

void main() {
    ivec2 d = ivec2(1, 0);
    ivec2 xy = ivec2(gl_FragCoord.xy);
    vec4 z    = texelFetch(uMainTexture, xy,        0);
    vec4 zp01 = texelFetch(uMainTexture, xy + d.yx, 0);
    vec4 zp10 = texelFetch(uMainTexture, xy + d.xy, 0);
    vec4 zm01 = texelFetch(uMainTexture, xy - d.yx, 0);
    vec4 zm10 = texelFetch(uMainTexture, xy - d.xy, 0);
    out_color = z * 0.5 + (zp01 + zp10 + zm01 + zm10) * 0.125;
    //out_color.r = 1.0;
    out_color.a = 1.0;
}
