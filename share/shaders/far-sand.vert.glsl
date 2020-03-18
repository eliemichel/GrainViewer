#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR PROCEDURAL_BASECOLOR2

layout (location = 0) in vec4 position;

out vec3 position_ws;
out vec3 baseColor;
out float radius;
out float screenSpaceRadius;

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
	vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);
	gl_Position = projectionMatrix * position_cs;

	screenSpaceRadius = SpriteSize_Botsch03(radius, position_cs);
	//screenSpaceRadius = SpriteSize(radius, gl_Position) * 0.5;
	//screenSpaceRadius = SpriteSize_Botsch03_corrected(radius, position_cs);

	gl_PointSize = screenSpaceRadius;

#ifdef PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;
#elif defined(PROCEDURAL_BASECOLOR2) // PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
	float u = pow(0.57 - position_ws.x * 0.12, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(u, 0.0)).rgb;
#else
    baseColor = vec3(1.0, 0.5, 0.0);
#endif // PROCEDURAL_BASECOLOR
}

