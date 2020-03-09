#version 450 core
#include "sys:defines"

// From MeshDataBehavior VAO
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv0;
layout (location = 4) in vec2 uv1;
layout (location = 5) in vec3 color0;

out vec3 position_ws;
out vec3 normal_ws;
out vec3 normal_cs;
out vec3 tangent_ws;
out vec2 uv0_ts;
out vec2 uv1_ts;
out vec3 color0_srgb;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

void main() {
	gl_Position = projectionMatrix * viewMatrix * vec4(position - vec3(4.1, 12.7, 2.4), 1.0);
	position_ws = (modelMatrix * vec4(position, 1.0)).xyz;
	normal_ws = mat3(modelMatrix) * normal;
	normal_cs = mat3(viewModelMatrix) * normal;
	tangent_ws = tangent;
	uv0_ts = uv0;
	uv1_ts = uv1;
	color0_srgb = color0;
}

