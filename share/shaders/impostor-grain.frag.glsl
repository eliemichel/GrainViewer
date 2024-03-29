#version 450 core
#include "sys:defines"

#pragma varopt PASS_BLIT_TO_MAIN_FBO PASS_SHADOW_MAP
#pragma opt PROCEDURAL_BASECOLOR
#pragma opt SET_DEPTH
#pragma opt NO_DISCARD
#pragma opt NO_INTERPOLATION
// if both MIXEDHIT_SAMPLING and SPHEREHIT_SAMPLING are defined, sampling will be mixed.
// NO_INTERPOLATION is defined only for default sampling (planeHit)
// SET_DEPTH allows for self intersection but significantly degrades performances

//#define MIXEDHIT_SAMPLING
//#define SPHEREHIT_SAMPLING
//#define PROCEDURAL_BASECOLOR

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(150.0/255.0, 231.0/255.0, 12.0/255.0);

///////////////////////////////////////////////////////////////////////////////
#if defined(PASS_BLIT_TO_MAIN_FBO)
// (pass called only if NO_DISCARD is on)

#define IN_LINEAR_GBUFFER
#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"
uniform sampler2D uFboDepthTexture;

// Problem: semi transparency requires to read from current color attachment
// but this creates dangerous feedback loops

#include "include/utils.inc.glsl"

// This stage gathers additive measures and turn them into regular g-buffer fragments
void main() {
	gl_FragDepth = texelFetch(uFboDepthTexture, ivec2(gl_FragCoord.xy), 0).x;

	GFragment fragment;
	autoUnpackLinearGFragment(fragment);
	if (fragment.alpha < 0.5) discard;
	float weightNormalization = 1.0 / fragment.alpha;

	fragment.baseColor *= weightNormalization;
	fragment.normal *= weightNormalization;
	fragment.material_id = pbrMaterial;
	fragment.alpha = 1.0;

	if (uDebugRenderType) {
		fragment.baseColor = uDebugRenderColor;
	}

	//fragment.baseColor = vec3(1.0, weightNormalization, 0.5);
	//fragment.material_id = forwardBaseColorMaterial;

	autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#else // PASS

in GeometryData {
	flat uint id;
	float radius;
	vec3 position_ws;
	mat4 gs_from_ws;
#ifdef PRECOMPUTE_IN_VERTEX
	flat uvec4 i;
	vec2 alpha;
#endif // PRECOMPUTE_IN_VERTEX
} geo;

// If noDiscard option is on, we write output in linear g-buffer because it is accumulated
#ifdef NO_DISCARD
#define OUT_LINEAR_GBUFFER
#else // NO_DISCARD
#define OUT_GBUFFER
#endif // NO_DISCARD
#include "include/gbuffer2.inc.glsl"

#ifdef SET_DEPTH
layout (depth_less) out float gl_FragDepth;
#endif // SET_DEPTH

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/impostor.inc.glsl"
#include "include/random.inc.glsl"
#include "include/depth.inc.glsl"
#include "grain/procedural-color.inc.glsl"

uniform SphericalImpostor uImpostor[3];
uniform float uGrainInnerRadiusRatio;

uniform float uHitSphereCorrectionFactor = .65;

uniform bool uHasMetallicRoughnessMap = false;
uniform float uDefaultRoughness = 0.5;
uniform float uDefaultMetallic = 0.0;

// Global switches
uniform int uDebugShape = -1; // 0: DEBUG_SPHERE 1: DEBUG_INNER_SPHERE 2: DEBUG_CUBE
uniform int uInterpolationMode = 1; // 0: NO_INTERPOLATION 1: INTERPOLATION
uniform int uSamplingMode = 0; // 0: PLANEHIT_SAMPLING 1: SPHEREHIT_SAMPLING 2: MIXEDHIT_SAMPLING

/**
 * Sample an impostor with different strategies depending on options
 */
GFragment SampleImpostor(const in SphericalImpostor impostor, const in Ray ray_gs, float radius) {
#ifdef NO_INTERPOLATION
#  ifdef PRECOMPUTE_IN_VERTEX
	return aux_IntersectRaySphericalGBillboardNoInterp(impostor, ray_gs, radius, geo.i.x);
#  else // PRECOMPUTE_IN_VERTEX
	return IntersectRaySphericalGBillboardNoInterp(impostor, ray_gs, radius);
#  endif // PRECOMPUTE_IN_VERTEX
#else // NO_INTERPOLATION
#  ifdef PRECOMPUTE_IN_VERTEX
	switch (uSamplingMode) {
	case 0: // PLANEHIT_SAMPLING
		return aux_IntersectRaySphericalGBillboard(impostor, ray_gs, radius, geo.i, geo.alpha);
	case 1: // SPHEREHIT_SAMPLING
		return aux_IntersectRaySphericalGBillboard_SphereHit(impostor, ray_gs, radius, uHitSphereCorrectionFactor, geo.i, geo.alpha);
	case 2: // MIXEDHIT_SAMPLING
		return aux_IntersectRaySphericalGBillboard_MixedHit(impostor, ray_gs, radius, uHitSphereCorrectionFactor, geo.i, geo.alpha);
	}
#  else // PRECOMPUTE_IN_VERTEX
	switch (uSamplingMode) {
	case 0: // PLANEHIT_SAMPLING
		return IntersectRaySphericalGBillboard(impostor, ray_gs, radius);
	case 1: // SPHEREHIT_SAMPLING
		return IntersectRaySphericalGBillboard_SphereHit(impostor, ray_gs, radius, uHitSphereCorrectionFactor);
	case 2: // MIXEDHIT_SAMPLING
		return IntersectRaySphericalGBillboard_MixedHit(impostor, ray_gs, radius, uHitSphereCorrectionFactor);
	}
#  endif // PRECOMPUTE_IN_VERTEX
#endif // NO_INTERPOLATION
}

/**
 * Pack the G-fragment depending on target type and options
 */
void pack(const in GFragment fragment) {
#ifdef SET_DEPTH
	vec3 p = (viewMatrix * vec4(fragment.ws_coord, 1.0)).xyz;
	setFragmentDepth(p);
#endif // SET_DEPTH

#ifdef NO_DISCARD
	autoPackLinearGFragment(fragment);
#else // NO_DISCARD
	autoPackGFragment(fragment);
#endif // NO_DISCARD
}

void main() {
	GFragment fragment;
	initGFragment(fragment);

	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
	Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

#if defined(PASS_SHADOW_MAP) && !defined(SET_DEPTH)
	// Early quit for shadow maps unless we alter fragment depth
	vec3 v;
	if (!intersectRaySphere(v, ray_ws, geo.position_ws.xyz, geo.radius)) {
		discard;
	}
	return;
#endif

	vec3 outerSphereHitPosition_ws;
	intersectRaySphere(outerSphereHitPosition_ws, ray_ws, geo.position_ws.xyz, geo.radius);

	switch (uDebugShape) {
	case 0: // DEBUG_SPHERE
		fragment = IntersectRaySphere(ray_ws, geo.position_ws, geo.radius);
		break;
	case 1: // DEBUG_INNER_SPHERE
		fragment = IntersectRaySphere(ray_ws, geo.position_ws, geo.radius * uGrainInnerRadiusRatio);
		break;
	case 2: // DEBUG_CUBE
		fragment = IntersectRayCube(ray_ws, geo.position_ws, geo.radius);
		break;
	default: // IMPOSTOR
		fragment = SampleImpostor(uImpostor[0], ray_gs, geo.radius);
		mat3 ws_from_gs_rot = transpose(mat3(geo.gs_from_ws));
		fragment.normal = ws_from_gs_rot * fragment.normal;
		break;
	}

#ifndef NO_DISCARD
	if (fragment.alpha < 0.5) discard;
#endif // not NO_DISCARD

	fragment.material_id = pbrMaterial;
	fragment.ws_coord = outerSphereHitPosition_ws; // for shadow maps

	// Color overrides
	if (uDebugRenderType) {
		fragment.baseColor = uDebugRenderColor;
	}
	else if (isUsingProceduralColor()) {
		fragment.baseColor = proceduralColor(geo.position_ws.xyz, geo.id);
		//float r = randomGrainColorFactor(int(geo.id));
		//float s = randomGrainColorFactor(int(geo.id) + 436);
		//fragment.baseColor += vec3(r - 0.1, s - 0.5, 0.0) * 0.05;
	}

	pack(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
