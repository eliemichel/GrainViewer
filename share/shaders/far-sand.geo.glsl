#version 450 core
#include "sys:defines"

#pragma variant STAGE_EPSILON_ZBUFFER
#pragma variant NO_DISCARD_IN_EPSILON_ZBUFFER
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
out float screenSpaceDiameter;

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

uniform bool uUseEarlyDepthTest;

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

// Early depth test
bool depthTest(vec4 position_clipspace) {
#ifdef STAGE_EPSILON_ZBUFFER
	return true;
#else // STAGE_EPSILON_ZBUFFER
	if (!uUseShellCulling || !uUseEarlyDepthTest) return true;
	vec3 fragCoord;
	vec2 res = resolution.xy - vec2(0.0, 0.0);
	fragCoord.xy = res * (position_clipspace.xy / position_clipspace.w * 0.5 + 0.5);
	fragCoord.xy = clamp(fragCoord.xy, vec2(0.5), res - vec2(0.5));
	fragCoord.z = position_clipspace.z / position_clipspace.w;
	float limitDepth = texelFetch(uDepthTexture, ivec2(fragCoord.xy), 0).x;

    return fragCoord.z <= limitDepth;
#endif // STAGE_EPSILON_ZBUFFER
}

void main() {
	if (!inBBox(inData[0].position_ws)) {
		return;
	}

	position_ws = inData[0].position_ws;	
	vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);

	vec4 offset = vec4(0.0);
#ifdef STAGE_EPSILON_ZBUFFER
	if (uShellCullingStrategy == cShellCullingMoveAway) {
		offset.xyz = uEpsilon * normalize(position_cs.xyz);
	}
#endif // STAGE_EPSILON_ZBUFFER

	vec4 position_clipspace = projectionMatrix * (position_cs + offset);

	if (!depthTest(position_clipspace)) {
		return;
	}

	gl_Position = position_clipspace;
	baseColor = computeBaseColor(position_ws);
	radius = inData[0].radius;
	screenSpaceDiameter = SpriteSize_Botsch03(radius, position_cs);
	//screenSpaceDiameter = SpriteSize(radius, gl_Position);
	//screenSpaceDiameter = SpriteSize_Botsch03_corrected(radius, position_cs);

#if defined(STAGE_EPSILON_ZBUFFER) && defined(NO_DISCARD_IN_EPSILON_ZBUFFER)
	// During Epsilon ZBuffer pass, we must not use discard in fragment shader
	// for performance reasons, and we prefer to have false negative than false
	// positive, hence we reduce the (square) point to fit inside the splatted
	// disc.
	gl_PointSize = screenSpaceDiameter * HALF_SQRT2;
#else // STAGE_EPSILON_ZBUFFER && NO_DISCARD_IN_EPSILON_ZBUFFER
	gl_PointSize = screenSpaceDiameter;
#endif // STAGE_EPSILON_ZBUFFER && NO_DISCARD_IN_EPSILON_ZBUFFER
	EmitVertex();
	EndPrimitive();
}

