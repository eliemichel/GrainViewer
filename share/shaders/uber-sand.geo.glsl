#version 450 core
#include "sys:defines"

#pragma varopt PASS_DEPTH PASS_EPSILON_DEPTH PASS_BLIT_TO_MAIN_FBO
#pragma varopt PROCEDURAL_BASECOLOR PROCEDURAL_BASECOLOR2 PROCEDURAL_BASECOLOR3 BLACK_BASECOLOR

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

#include "include/standard-posteffect.geo.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

layout(points) in;
layout(points, max_vertices = 1) out;

in VertexData {
	vec3 position_ws;
	vec3 originalPosition_ws; // for procedural color
	float radius;
	uint vertexId;
} inData[];

out FragmentData {
	vec3 position_ws;
	vec3 baseColor;
	float radius;
	float screenSpaceDiameter;
	float metallic;
	float roughness;
} outData;

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/sprite.inc.glsl"
#include "include/random.inc.glsl"
#include "include/zbuffer.inc.glsl"
#include "sand/procedural-color.inc.glsl"

#include "include/raytracing.inc.glsl"
#include "include/gbuffer2.inc.glsl"
#include "include/impostor.inc.glsl"
uniform SphericalImpostor uImpostor[3];

uniform bool uUseShellCulling = true;
uniform sampler2D uDepthTexture;
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
#ifdef USING_PRECEDURAL_COLOR
	outData.baseColor = proceduralColor(inData[0].originalPosition_ws, inData[0].vertexId);
#else // USING_PRECEDURAL_COLOR
	mat4 gs_from_ws = randomGrainMatrix(int(inData[0].vertexId), outData.position_ws);
	vec3 ray_ws_origin = (inverseViewMatrix * vec4(0, 0, 0, 1)).xyz;
	vec3 ray_ws_direction = outData.position_ws - ray_ws_origin;
	vec3 ray_gs_direction = normalize(mat3(gs_from_ws) * ray_ws_direction);

	uint n = uImpostor[0].viewCount;
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray_gs_direction, n, i, alpha);
	vec2 calpha = vec2(1.) - alpha;

	// base color
	if (uImpostor[0].hasBaseColorMap) {
		int level = textureQueryLevels(uImpostor[0].baseColorTexture);
		mat4 colors = mat4(
			texelFetch(uImpostor[0].baseColorTexture, ivec3(0, 0, int(i.x)), level - 1),
			texelFetch(uImpostor[0].baseColorTexture, ivec3(0, 0, int(i.y)), level - 1),
			texelFetch(uImpostor[0].baseColorTexture, ivec3(0, 0, int(i.z)), level - 1),
			texelFetch(uImpostor[0].baseColorTexture, ivec3(0, 0, int(i.w)), level - 1)
		);
		vec4 c = colors * vec4(vec2(calpha.x, alpha.x) * calpha.y, vec2(calpha.x, alpha.x) * alpha.y);
		outData.baseColor = c.rgb;
	} else {
		outData.baseColor = uImpostor[0].baseColor;
	}

	// metallic/roughness
	if (uImpostor[0].hasMetallicRoughnessMap) {
		int level = textureQueryLevels(uImpostor[0].metallicRoughnessTexture);
		mat4 colors = mat4(
			texelFetch(uImpostor[0].metallicRoughnessTexture, ivec3(0, 0, int(i.x)), level - 1),
			texelFetch(uImpostor[0].metallicRoughnessTexture, ivec3(0, 0, int(i.y)), level - 1),
			texelFetch(uImpostor[0].metallicRoughnessTexture, ivec3(0, 0, int(i.z)), level - 1),
			texelFetch(uImpostor[0].metallicRoughnessTexture, ivec3(0, 0, int(i.w)), level - 1)
		);
		vec4 c = colors * vec4(vec2(calpha.x, alpha.x) * calpha.y, vec2(calpha.x, alpha.x) * alpha.y);
		outData.metallic = c.x;
		outData.roughness = c.y;
	} else {
		outData.metallic = uImpostor[0].metallic;
		outData.roughness = uImpostor[0].roughness;
	}

#endif // USING_PRECEDURAL_COLOR
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
