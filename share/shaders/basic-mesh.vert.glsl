#version 450 core
#include "sys:defines"

// From MeshDataBehavior VAO
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint materialId;
layout (location = 4) in vec3 tangent;

out vec3 position_ws;
out vec3 normal_ws;
out vec3 normal_cs;
out vec3 tangent_ws;
out vec2 uv_ts;
flat out uint matId;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

void main() {
	gl_Position = projectionMatrix * viewModelMatrix * vec4(position, 1.0);
	position_ws = (modelMatrix * vec4(position, 1.0)).xyz;
	normal_ws = mat3(modelMatrix) * normal;
	normal_cs = mat3(viewModelMatrix) * normal;
	tangent_ws = tangent;
	uv_ts = uv;
	matId = materialId;
}

