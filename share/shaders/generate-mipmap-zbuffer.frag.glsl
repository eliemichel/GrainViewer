#version 450 core
#include "sys:defines"

uniform sampler2D previousLevel;

void main() {
    gl_FragDepth = 1.0;
}
