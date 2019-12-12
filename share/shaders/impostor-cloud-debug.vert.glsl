#version 430 core

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer points {
	PointCloundVboEntry vbo[];
};

flat out uint pointId;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/anim.inc.glsl"

void main() {
	pointId = gl_VertexID;
	uint animPointId = AnimatedPointId(pointId, uFrameCount, uPointCount, uTime, uFps);
    vec3 p = vbo[animPointId].position.xyz;

    gl_Position = vec4(p, 1.0);
}
