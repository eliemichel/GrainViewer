#version 450 core
#include "sys:defines"

// From MeshDataBehavior VAO
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint materialId;
layout (location = 4) in vec3 tangent;

out GeometryData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
} geo;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

void main() {
	gl_Position = projectionMatrix * viewModelMatrix * vec4(position, 1.0);
	geo.position_ws = (modelMatrix * vec4(position, 1.0)).xyz;
	geo.normal_ws = mat3(modelMatrix) * normal;
	geo.tangent_ws = tangent;
	geo.uv = vec2(uv.x, 1.-uv.y);
	geo.materialId = 0;//materialId;
}

