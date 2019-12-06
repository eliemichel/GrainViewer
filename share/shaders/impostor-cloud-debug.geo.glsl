#version 450 core
#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

flat in uint pointId[];

out float radius;
out vec3 position_ws;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform float grainRadius;

#include "include/utils.inc.glsl"
#include "include/sprite.inc.glsl"

void main() {
	radius = grainRadius;

    position_ws = (modelMatrix * gl_in[0].gl_Position).xyz;
    vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(radius, gl_Position);

    EmitVertex();
    EndPrimitive();
}
