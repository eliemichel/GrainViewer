#version 450 core
#include "sys:defines"

layout (location = 0) in vec3 position;

out vec3 position_ws;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform mat4 lightProjectionMatrix;
uniform mat4 lightInverseViewMatrix;

#include "include/utils.inc.glsl"

void main() {
	vec3 v = position;

	float n, f, l, r, b, t;
	if (isOrthographic(lightProjectionMatrix)) {
		n = (lightProjectionMatrix[3][2] - 1) / lightProjectionMatrix[2][2];
		f = (lightProjectionMatrix[3][2] - 1) / lightProjectionMatrix[2][2];
		l = -(lightProjectionMatrix[3][0] - 1) / lightProjectionMatrix[0][0];
		r = -(lightProjectionMatrix[3][0] + 1) / lightProjectionMatrix[0][0];
		b = -(lightProjectionMatrix[3][1] - 1) / lightProjectionMatrix[1][1];
		t = -(lightProjectionMatrix[3][1] + 1) / lightProjectionMatrix[1][1];

		v = vec3(mix(l, r, v.x), mix(b, t, v.y), -mix(n, f, v.y));
	}
	else {
		n = lightProjectionMatrix[3][2] / (lightProjectionMatrix[2][2] - 1);
		f = lightProjectionMatrix[3][2] / (lightProjectionMatrix[2][2] + 1);
		l = (lightProjectionMatrix[2][0] - 1) / lightProjectionMatrix[0][0];
		r = (lightProjectionMatrix[2][0] + 1) / lightProjectionMatrix[0][0];
		b = (lightProjectionMatrix[2][1] - 1) / lightProjectionMatrix[1][1];
		t = (lightProjectionMatrix[2][1] + 1) / lightProjectionMatrix[1][1];

		float s = mix(n, f, v.z);
		v = vec3(mix(l, r, v.x) * s, mix(b, t, v.y) * s, -s);
	}

	position_ws = (modelMatrix * lightInverseViewMatrix * vec4(v, 1.0)).xyz;
	gl_Position = projectionMatrix * viewMatrix * vec4(position_ws, 1.0);
}

