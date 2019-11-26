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
out vec3 baseColor;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform sampler2D colormapTexture;
uniform float grainRadius = 0.005;
uniform float grainMeshScale = 4.5;

#include "include/random.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
    uint pointId = elements[gl_InstanceID];
    vec3 grainCenter_ws = (modelMatrix * vbo[pointId].position).xyz;

    mat3 ws_from_gs = transpose(mat3(randomGrainMatrix(int(pointId), grainCenter_ws)));
    
	vec4 p = vec4(ws_from_gs * position * grainRadius * grainMeshScale + grainCenter_ws, 1.0);

    position_ws = p.xyz;
    normal_ws = ws_from_gs * normal;

    gl_Position = projectionMatrix * viewMatrix * vec4(position_ws, 1.0);
    
	float r = randomGrainColorFactor(int(pointId));
    baseColor = texture(colormapTexture, vec2(r, 0.0)).rgb;
}
