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
layout(std430, binding = 1) buffer pointsSsbo {
    PointCloundVboEntry vbo[];
};
layout (std430, binding = 2) buffer elementsSsbo {
    uint elements[];
};

out vec3 normal_ws;
out vec3 position_ws;
out vec3 tangent_ws;
out vec2 uv_ts;
flat out uint matId;
out vec3 baseColor;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#pragma variant PROCEDURAL_BASECOLOR

uniform sampler2D colormapTexture;
uniform float grainRadius = 0.005;
uniform float grainMeshScale = 4.5;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

#include "include/random.inc.glsl"
#include "include/anim.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
    uint pointId = elements[gl_InstanceID];
    uint animPointId = AnimatedPointId(pointId, uFrameCount, uPointCount, uTime, uFps);

    vec3 grainCenter_ws = (modelMatrix * vec4(vbo[animPointId].position.xyz, 1.0)).xyz;

    mat3 ws_from_gs = transpose(mat3(randomGrainMatrix(int(pointId), grainCenter_ws)));
    
	vec4 p = vec4(ws_from_gs * position * grainRadius * grainMeshScale + grainCenter_ws, 1.0);

    position_ws = p.xyz;
    normal_ws = ws_from_gs * normal;

    gl_Position = projectionMatrix * viewMatrix * vec4(position_ws, 1.0);
    
    tangent_ws = tangent;
    uv_ts = vec2(uv.x, 1.-uv.y);
    matId = 0;

#ifdef PROCEDURAL_BASECOLOR
	float r = randomGrainColorFactor(int(pointId));
    baseColor = texture(colormapTexture, vec2(r, 0.0)).rgb;
#endif // PROCEDURAL_BASECOLOR
}
