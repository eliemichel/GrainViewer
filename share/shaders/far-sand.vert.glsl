#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

layout (location = 0) in vec4 position;

out vec3 position_ws;
out vec3 baseColor;
out float radius;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/sprite.inc.glsl"
#include "include/random.inc.glsl"
#include "sand/random-grains.inc.glsl"

uniform float uRadius = 0.005;
uniform sampler2D uColormapTexture;

void main() {
	radius = uRadius;
	position_ws = (modelMatrix * vec4(position.xyz, 1.0)).xyz;
	gl_Position = projectionMatrix * viewMatrix * vec4(position_ws, 1.0);
	gl_PointSize = SpriteSize(radius, gl_Position);

#ifdef PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;
#else // PROCEDURAL_BASECOLOR
    baseColor = vec3(1.0, 0.5, 0.0);
#endif // PROCEDURAL_BASECOLOR
}

