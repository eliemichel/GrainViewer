#version 450 core
#include "sys:defines"

#pragma variant STAGE_EPSILON_ZBUFFER
#pragma variant PROCEDURAL_BASECOLOR PROCEDURAL_BASECOLOR2 PROCEDURAL_BASECOLOR3 BLACK_BASECOLOR

layout(points) in;
layout(points, max_vertices = 1) out;

in VertexData {
	vec3 position_ws;
#ifdef PROCEDURAL_BASECOLOR3
	vec3 originalPosition_ws;
#endif // PROCEDURAL_BASECOLOR3
	float radius;
	uint vertexId;
} inData[];

out vec3 position_ws;
out vec3 baseColor;
out float radius;

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/sprite.inc.glsl"
#include "include/random.inc.glsl"
#include "include/zbuffer.inc.glsl"
#include "sand/random-grains.inc.glsl"

uniform bool uUseShellCulling = true;
uniform sampler2D uDepthTexture;
uniform sampler2D uColormapTexture;
uniform bool uUseBbox = false;
uniform vec3 uBboxMin;
uniform vec3 uBboxMax;
uniform float uEpsilon;

const int cShellCullingFragDepth = 0;
const int cShellCullingMoveAway = 1;
const int cShellCullingDepthRange = 2;
uniform int uShellCullingStrategy;

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
	uint id = inData[0].vertexId;
	float r = randomGrainColorFactor(int(id));
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;
#elif defined(PROCEDURAL_BASECOLOR2) // PROCEDURAL_BASECOLOR
	uint id = inData[0].vertexId;
	float r = randomGrainColorFactor(int(id));
	float u = pow(0.57 - pos.x * 0.12, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(u, 0.0)).rgb;
#elif defined(PROCEDURAL_BASECOLOR3) // PROCEDURAL_BASECOLOR
	uint id = inData[0].vertexId;
	float r = randomGrainColorFactor(int(id));
	float u = pow(0.5 + pos.z * 0.5, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(clamp(u + (r - 0.5) * 0.2, 0.01, 0.99), 0.0)).rgb;

    // Add a blue dot
    float th = 0.01;
    if (abs(atan(pos.y, pos.x)) < th && abs(atan(pos.z, pos.x)) < th) {
	    baseColor = vec3(0.0, 0.2, 0.9);
	}

	baseColor = mix(baseColor, baseColor.bgr, smoothstep(0.75, 0.73, length(inData[0].originalPosition_ws)));
#elif defined(BLACK_BASECOLOR)
	baseColor = vec3(0.0, 0.0, 0.0);
#else
    baseColor = vec3(1.0, 0.5, 0.0);
#endif // PROCEDURAL_BASECOLOR
	return baseColor;
}

bool depthTest(vec4 position_clipspace) {
#ifdef STAGE_EPSILON_ZBUFFER
	return true;
#else // STAGE_EPSILON_ZBUFFER
	return true; // TODO: early depth test
	if (!uUseShellCulling) return true;
	vec3 fragCoord;
	fragCoord.xy = resolution.xy * (position_clipspace.xy / position_clipspace.w * 0.5 + 0.5);
	fragCoord.z = position_clipspace.z / position_clipspace.w;
	float d = texelFetch(uDepthTexture, ivec2(fragCoord.xy), 0).x;
    float limitDepth = linearizeDepth(d);
    float depth = /*linearizeDepth*/(fragCoord.z);
    return depth >= limitDepth;
#endif // STAGE_EPSILON_ZBUFFER
}

void main() {
	if (!inBBox(inData[0].position_ws)) {
		return;
	}

	position_ws = inData[0].position_ws;	
	vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);

#ifdef STAGE_EPSILON_ZBUFFER
	if (uShellCullingStrategy == cShellCullingMoveAway) {
		position_cs.xyz += uEpsilon / length(position_cs) * position_cs.xyz;
		//position_cs.z -= 0.1;
	}
#endif // STAGE_EPSILON_ZBUFFER

	vec4 position = projectionMatrix * position_cs;

	if (!depthTest(position)) {
		return;
	}

	gl_Position = position;
	baseColor = computeBaseColor(position_ws);
	radius = inData[0].radius;
	float screenSpaceDiameter = SpriteSize_Botsch03(radius, position_cs);
	//screenSpaceDiameter = SpriteSize(radius, gl_Position);
	//screenSpaceDiameter = SpriteSize_Botsch03_corrected(radius, position_cs);
	gl_PointSize = screenSpaceDiameter;
	EmitVertex();
	EndPrimitive();
}
