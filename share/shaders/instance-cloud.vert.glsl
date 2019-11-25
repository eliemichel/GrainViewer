#version 450 core

struct InstanceData {
	uint index;
	vec3 position;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 raw_instance;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 1) buffer pointsSsbo {
    PointCloundVboEntry vbo[];
};
layout (std430, binding = 2) buffer elementsSsbo {
    uint elements[];
};

out vec4 grainAlbedo;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform vec3 lightPos = vec3(-10.f, 10.f, 10.f);
uniform vec3 lightPos2 = vec3(10.f, -5.f, -10.f);


float randv2(vec2 co){
    return fract(sin(dot(co ,vec2(12.9898,78.233))) * 43758.5453);
}

InstanceData vec4ToInstanceData(vec4 v) {
	return InstanceData(floatBitsToUint(v.x), v.yzw);
}

void main() {
	InstanceData instance = vec4ToInstanceData(raw_instance);

    uint pointId = elements[gl_InstanceID];
    vec3 center = vbo[pointId].position.xzy;

	vec4 p = vec4(position * 0.02 + center, 1.0);

    gl_Position = projectionMatrix * viewModelMatrix * p;
    
	float f = float(pointId + 1) * 0.1 + 1;
    float fac = randv2(vec2(2*f, 2 + f));
    grainAlbedo = mix(
        mix(vec4(1.278, 0.878, 0.278, 0.7), vec4(0.902, 0.831, 0.702, 0.2), fac * 2.0),
        mix(vec4(0.902, 0.831, 0.702, 0.2), vec4(1.0, 1.0, 1.0, 0.0), fac * 2.0 - 0.5),
        clamp((fac - 0.5) * 1000.0, 0.0, 1.0)
        );
}
