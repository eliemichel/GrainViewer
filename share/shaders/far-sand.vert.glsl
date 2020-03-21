#version 450 core
#include "sys:defines"

layout (location = 0) in vec4 position;

out VertexData {
	vec3 position_ws;
	float radius;
} outData;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;

uniform float uRadius = 0.005;
uniform sampler2D uColormapTexture;

void main() {
	outData.radius = uRadius;
	outData.position_ws = (modelMatrix * vec4(position.xyz, 1.0)).xyz;
}

