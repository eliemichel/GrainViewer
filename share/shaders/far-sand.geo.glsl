#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR PROCEDURAL_BASECOLOR2 PROCEDURAL_BASECOLOR3 BLACK_BASECOLOR

layout(points) in;
layout(points, max_vertices = 1) out;

in VertexData {
	vec3 position_ws;
	float radius;
} inData[];

out vec3 position_ws;
out vec3 baseColor;
out float radius;

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/sprite.inc.glsl"
#include "include/random.inc.glsl"
#include "sand/random-grains.inc.glsl"

uniform bool uUseBbox = false;
uniform vec3 uBboxMin;
uniform vec3 uBboxMax;

bool inBBox(vec3 pos) {
	if (!uUseBbox) return true;
	else return (
		pos.x >= uBboxMin.x && pos.x <= uBboxMax.x &&
		pos.y >= uBboxMin.y && pos.y <= uBboxMax.y &&
		pos.z >= uBboxMin.z && pos.z <= uBboxMax.z
	);
}

vec3 computeBaseColor(vec3 pos) {
	vec3 baseColor;
#ifdef PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;
#elif defined(PROCEDURAL_BASECOLOR2) // PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
	float u = pow(0.57 - pos.x * 0.12, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(u, 0.0)).rgb;
#elif defined(PROCEDURAL_BASECOLOR3) // PROCEDURAL_BASECOLOR
	uint id = gl_VertexID;
	float r = randomGrainColorFactor(int(id));
	float u = pow(0.5 + pos.z * 0.5, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(clamp(u + (r - 0.5) * 0.2, 0.01, 0.99), 0.0)).rgb;

    // Add a blue dot
    float th = 0.1;
    if (abs(atan(pos.y, pos.x)) < th && abs(atan(pos.z, pos.x)) < th) {
	    baseColor = vec3(0.0, 0.2, 0.9);
	}
#elif defined(BLACK_BASECOLOR)
	baseColor = vec3(0.0, 0.0, 0.0);
#else
    baseColor = vec3(1.0, 0.5, 0.0);
#endif // PROCEDURAL_BASECOLOR
	return baseColor;
}

void main() {
	if (inBBox(inData[0].position_ws)) {
		position_ws = inData[0].position_ws;
		baseColor = computeBaseColor(position_ws);
		radius = inData[0].radius;
		
		vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);
		gl_Position = projectionMatrix * position_cs;

		float screenSpaceDiameter = SpriteSize_Botsch03(radius, position_cs);
		//screenSpaceDiameter = SpriteSize(radius, gl_Position);
		//screenSpaceDiameter = SpriteSize_Botsch03_corrected(radius, position_cs);
		gl_PointSize = screenSpaceDiameter;
		EmitVertex();
	}
	EndPrimitive();
}

