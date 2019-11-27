#version 330 core

layout (location = 0) in vec3 position;

out vec3 direction;

#include "include/uniform/camera.inc.glsl"

void main() {
	vec4 pos = projectionMatrix * mat4(mat3(viewMatrix)) * vec4(position, 0.001f);
    gl_Position = pos.xyww;
    direction = position;
}
