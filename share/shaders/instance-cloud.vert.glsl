#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 1) buffer pointsSsbo {
    PointCloundVboEntry vbo[];
};
layout (std430, binding = 2) buffer elementsSsbo {
    uint elements[];
};

out vec3 normal_ws;
out vec3 position_ws;
out vec4 grainAlbedo;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform vec3 lightPos = vec3(-10.f, 10.f, 10.f);
uniform vec3 lightPos2 = vec3(10.f, -5.f, -10.f);

#include "include/random.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
    uint pointId = elements[gl_InstanceID];
    vec3 grainCenter_ws = vbo[pointId].position.xzy;

    mat3 ws_from_gs = transpose(mat3(randomGrainMatrix(int(pointId), grainCenter_ws)));
    
	vec4 p = vec4(ws_from_gs * position * 0.0225 + grainCenter_ws, 1.0);

    position_ws = p.xyz;
    normal_ws = ws_from_gs * mat3(modelMatrix) * normal;

    gl_Position = projectionMatrix * viewMatrix * vec4(position_ws, 1.0);
    
	float f = float(pointId + 1) * 0.1 + 1;
    float fac = randv2(vec2(2*f, 2 + f));
    grainAlbedo = mix(
        mix(vec4(1.278, 0.878, 0.278, 0.7), vec4(0.902, 0.831, 0.702, 0.2), fac * 2.0),
        mix(vec4(0.902, 0.831, 0.702, 0.2), vec4(1.0, 1.0, 1.0, 0.0), fac * 2.0 - 0.5),
        clamp((fac - 0.5) * 1000.0, 0.0, 1.0)
        );
}
