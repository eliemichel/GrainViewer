#version 450 core
#include "sys:defines"

#pragma varopt PASS_DEPTH PASS_EPSILON_DEPTH PASS_BLIT_TO_MAIN_FBO
#pragma variant PROCEDURAL_BASECOLOR PROCEDURAL_BASECOLOR2 PROCEDURAL_BASECOLOR3 BLACK_BASECOLOR

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO
#extension GL_EXT_geometry_shader4 : enable // to use gl_PositionIn[]

in vec2 vertUv[];
out vec2 uv;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// this is just a passthrough geo shader
void main() {
	for (int i = 0; i < gl_VerticesIn; i++) {
		gl_Position = gl_PositionIn[i];
		uv = vertUv[i];
		EmitVertex();
	}
	EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

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

out FragmentData {
	vec3 position_ws;
	vec3 baseColor;
	float radius;
	float screenSpaceDiameter;
} outData;

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
#if defined(PASS_DEPTH) || defined(PASS_EPSILON_DEPTH)
	return true;
#else // PASS_DEPTH
	if (!uUseShellCulling || !uUseEarlyDepthTest) return true;
	vec3 fragCoord;
	vec2 res = resolution.xy - vec2(0.0, 0.0);
	fragCoord.xy = res * (position_clipspace.xy / position_clipspace.w * 0.5 + 0.5);
	fragCoord.xy = clamp(fragCoord.xy, vec2(0.5), res - vec2(0.5));
	fragCoord.z = position_clipspace.z / position_clipspace.w;
	float limitDepth = texelFetch(uDepthTexture, ivec2(fragCoord.xy), 0).x;

	return fragCoord.z <= limitDepth;
#endif // PASS_DEPTH
}

void main() {
	if (!inBBox(inData[0].position_ws)) {
		return;
	}

	outData.position_ws = inData[0].position_ws;	
	vec4 position_cs = viewMatrix * vec4(outData.position_ws, 1.0);

	vec4 offset = vec4(0.0);
#ifdef PASS_EPSILON_DEPTH
	// Move away to let a shell of thickness uEpsilon
	offset.xyz = uEpsilon * normalize(position_cs.xyz);
#endif // PASS_EPSILON_DEPTH

	vec4 position_clipspace = projectionMatrix * (position_cs + offset);

	if (!depthTest(position_clipspace)) {
		return;
	}

	gl_Position = position_clipspace;
	outData.baseColor = computeBaseColor(outData.position_ws);
	outData.radius = inData[0].radius;
	outData.screenSpaceDiameter = SpriteSize_Botsch03(outData.radius, position_cs);
	//outData.screenSpaceDiameter = SpriteSize(outData.radius, gl_Position);
	//outData.screenSpaceDiameter = SpriteSize_Botsch03_corrected(outData.radius, position_cs);

	gl_PointSize = outData.screenSpaceDiameter;
	EmitVertex();
	EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS_BLIT_TO_MAIN_FBO
