#version 450 core
#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

flat in int vId[];

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

void main() {
    vec4 position_cs = viewModelMatrix * gl_in[0].gl_Position;
    gl_Position = projectionMatrix * position_cs;
    gl_PointSize = 2.0;

    EmitVertex();
    EndPrimitive();
}
