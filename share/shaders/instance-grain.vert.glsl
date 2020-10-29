#version 450 core
#include "sys:defines"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint materialId;
layout (location = 4) in vec3 tangent;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) restrict readonly buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) restrict readonly buffer pointElementsSsbo {
    uint pointElements[];
};

out VertexData {
    vec3 normal_ws;
    vec3 position_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
    vec3 baseColor;
} vert;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#pragma variant PROCEDURAL_BASECOLOR

uniform float uGrainRadius = 0.005;
uniform float uGrainMeshScale = 4.5;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

uniform bool uUseAnimation = true;
uniform bool uUsePointElements = true;

#include "include/random.inc.glsl"
#include "include/anim.inc.glsl"
#include "grain/procedural-color.inc.glsl"

void main() {
    uint pointId =
        uUsePointElements
        ? pointElements[gl_InstanceID]
        : gl_InstanceID;

    uint animPointId =
        uUseAnimation
        ? AnimatedPointId2(pointId, uFrameCount, uPointCount, uTime, uFps)
        : pointId;

    vec3 grainCenter_ws = (modelMatrix * vec4(pointVertexAttributes[animPointId].position.xyz, 1.0)).xyz;

    pointId = animPointId%20; // WTF?
    mat3 ws_from_gs = transpose(mat3(randomGrainMatrix(int(pointId), grainCenter_ws)));
    
    vec3 vertexPosition = position;
	vec4 p = vec4(ws_from_gs * vertexPosition * uGrainRadius * uGrainMeshScale + grainCenter_ws, 1.0);

    vert.position_ws = p.xyz;
    vert.normal_ws = ws_from_gs * normal;
    vert.tangent_ws = tangent;
    vert.uv = vec2(uv.x, 1.-uv.y);
    vert.materialId = materialId;
    vert.baseColor = proceduralColor(vert.position_ws, pointId);
    
    gl_Position = projectionMatrix * viewMatrix * vec4(vert.position_ws, 1.0);
}
