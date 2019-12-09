#version 450 core
#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

flat in uint pointId[];

flat out uint id;
out float radius;
out vec3 position_ws;
out mat4 gs_from_ws;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform float uOuterRadius = 0.005;

#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"
#include "include/sprite.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
	id = pointId[0];
	radius = uOuterRadius;

    position_ws = (modelMatrix * gl_in[0].gl_Position).xyz;
    vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(radius, gl_Position);

    gs_from_ws = randomGrainMatrix(int(id), position_ws);

    EmitVertex();
    EndPrimitive();
}
