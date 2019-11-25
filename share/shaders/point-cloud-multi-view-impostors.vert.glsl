#version 430 core

layout (location = 0) in vec3 position;

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) buffer points {
	PointCloundVboEntry vbo[];
};

flat out int vId;

uniform uint uFrameCount = 136;
uniform uint uPointCount = 15000;
uniform float uFps = 25.0;
uniform float time;

void main() {
    //vec3 p = vec3(position.x, position.z, position.y);
    uint frame = uint(time * uFps) % uFrameCount;
	vec3 p = vbo[gl_VertexID + uPointCount*frame].position.xyz;
    //gl_Position = vec4(p / 20.f, 1.0);
    gl_Position = vec4(p, 1.0);
    vId = gl_VertexID;
}
