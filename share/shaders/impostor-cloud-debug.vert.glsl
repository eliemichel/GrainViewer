#version 430 core

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer points {
	PointCloundVboEntry vbo[];
};

flat out uint pointId;

uniform uint uFrameCount = 136;
uniform uint uPointCount = 15000;
uniform float uFps = 25.0;
uniform float time;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

void main() {
    uint frame = uint(time * uFps) % uFrameCount;
	//vec3 p = vbo[gl_VertexID + uPointCount*frame].position.xyz;
	pointId = gl_VertexID;
    vec3 p = vbo[pointId].position.xyz;

    gl_Position = vec4(p, 1.0);
}