#version 450 core
#include "sys:defines"

// From MeshDataBehavior VAO
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint materialId;
layout (location = 4) in vec3 tangent;

out VertexData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
    flat int layer;
} vert;

uniform mat4 uModelMatrix;

void main() {
	vert.position_ws = (uModelMatrix * vec4(position, 1.0)).xyz;
	vert.normal_ws = mat3(uModelMatrix) * normal;
	vert.tangent_ws = tangent;
	vert.uv = vec2(uv.x, 1.-uv.y);
	vert.materialId = materialId;
	vert.layer = gl_InstanceID;
}

