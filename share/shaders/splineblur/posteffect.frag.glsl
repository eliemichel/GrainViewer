#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 out_color;

in vec2 uv;

uniform sampler2D uMainTexture;

uniform float uWidth;
uniform float uHeight;

void main() {
    ivec2 xy = ivec2(gl_FragCoord.xy);
    out_color = texelFetch(uMainTexture, xy, 0);
    out_color.r /= uWidth;
    out_color.g /= uHeight;
    //out_color.b /= max(uWidth, uHeight) * 1.72 * 0.1;
}
