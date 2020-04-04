#version 450 core
#include "sys:defines"

#pragma opt PROCEDURAL_BASECOLOR
#pragma opt SET_DEPTH
#pragma opt SHADOW_MAP
// if both MIXEDHIT_SAMPLING and SPHEREHIT_SAMPLING are defined, sampling will be mixed.
// NO_INTERPOLATION is defined only for default sampling (planeHit)
// SET_DEPTH allows for self intersection but significantly degrades performances

//#define MIXEDHIT_SAMPLING
//#define SPHEREHIT_SAMPLING

in GeometryData {
	flat uint id;
	float radius;
	vec3 position_ws;
	mat4 gs_from_ws;
} geo;

#define OUT_GBUFFER
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
#include "sand/procedural-color.inc.glsl"

uniform SphericalImpostor impostor[3];
uniform float uGrainInnerRadiusRatio;

uniform float uHitSphereCorrectionFactor = .65;

uniform bool uHasMetallicRoughnessMap = false;
uniform float uDefaultRoughness = 0.5;
uniform float uDefaultMetallic = 0.0;

// Global switches
uniform int uDebugShape = -1; // 0: DEBUG_SPHERE 1: DEBUG_INNER_SPHERE 2: DEBUG_CUBE
uniform int uInterpolationMode = 1; // 0: NO_INTERPOLATION 1: INTERPOLATION
uniform int uSamplingMode = 0; // 0: PLANEHIT_SAMPLING 1: SPHEREHIT_SAMPLING 2: MIXEDHIT_SAMPLING
uniform bool uDebugRenderType = false;

uniform vec3 uDebugRenderColor = vec3(150.0/255.0, 231.0/255.0, 12.0/255.0);

void main() {
	GFragment fragment;
	initGFragment(fragment);

	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);

	vec3 outerSphereHitPosition_ws;
	intersectRaySphere(outerSphereHitPosition_ws, ray_ws, geo.position_ws.xyz, geo.radius);

	mat3 ws_from_gs_rot = transpose(mat3(geo.gs_from_ws));
    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);
    vec3 position_gs = (geo.gs_from_ws * vec4(geo.position_ws, 1.0)).xyz;

    //fragment = IntersectRaySphericalGBillboardNoInterp(impostor[0], ray_gs, position_gs, geo.radius);
    fragment = IntersectRaySphericalGBillboard(impostor[0], ray_gs, position_gs, geo.radius);

    //if (fragment.alpha < 0.5) discard;

	//fragment.baseColor = position_gs.xyz + ray_gs.direction.xyz * 0.5;
	fragment.material_id = forwardNormalMaterial;
	autoPackGFragment(fragment);
	return;
#if 0
	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
	vec3 innerSphereHitPosition_ws;

#ifdef SHADOW_MAP
#ifndef SET_DEPTH
	// Early quit for shadow maps unless we alter fragment depth
	bool hit = intersectRaySphere(innerSphereHitPosition_ws, ray_ws, geo.position_ws.xyz, geo.radius*2);
	if (!hit) {
		discard;
	}
	return;
#endif
#endif

	float innerRadius = geo.radius * uGrainInnerRadiusRatio;

	bool intersectInnerSphere = intersectRaySphere(innerSphereHitPosition_ws, ray_ws, geo.position_ws.xyz, innerRadius);
	vec3 outerSphereHitPosition_ws;
	intersectRaySphere(outerSphereHitPosition_ws, ray_ws, geo.position_ws.xyz, geo.radius);

	mat3 ws_from_gs_rot = transpose(mat3(geo.gs_from_ws));

    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

    vec3 position_gs = (geo.gs_from_ws * vec4(geo.position_ws, 1.0)).xyz;

    GFragment fragment;
	if (uInterpolationMode == 0) {
		fragment = IntersectRaySphericalGBillboardNoInterp(impostor[0], ray_gs, position_gs, geo.radius);
	} else {
		switch (uSamplingMode) {
		case 0: // PLANEHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard(impostor[0], ray_gs, position_gs, geo.radius);
			break;
		case 1: // SPHEREHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard_SphereHit(impostor[0], ray_gs, position_gs, geo.radius, uHitSphereCorrectionFactor);
			break;
		case 2: // MIXEDHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard_MixedHit(impostor[0], ray_gs, position_gs, geo.radius, uHitSphereCorrectionFactor);
			break;
		}
	}
	
	switch (uDebugShape) {
	case 0: // DEBUG_SPHERE
		fragment = IntersectRaySphere(ray_ws, geo.position_ws, geo.radius);
		break;
	case 1: // DEBUG_INNER_SPHERE
		fragment = IntersectRaySphere(ray_ws, geo.position_ws, innerRadius);
		break;
	case 2: // DEBUG_CUBE
		fragment = IntersectRayCube(ray_ws, geo.position_ws, geo.radius);
		break;
	default:
		fragment.normal = ws_from_gs_rot * fragment.normal;
		break;
	}

	//fragment.ws_coord = ws_from_gs_rot * (fragment.ws_coord - position_gs) + geo.position_ws;
	//fragment.ws_coord = innerSphereHitPosition_ws;
	fragment.ws_coord = outerSphereHitPosition_ws;
	fragment.material_id = pbrMaterial;

	if (fragment.alpha < 0.5) discard;
	//if (dot(fragment.normal, ray_ws.direction) >= 0.0) discard;

	if (isUsingProceduralColor()) {
		fragment.baseColor = proceduralColor(geo.position_ws.xyz, geo.id);
	}

	if (!impostor[0].hasMetallicRoughnessMap) {
		fragment.metallic = uDefaultMetallic;
		fragment.roughness = uDefaultRoughness;
	}

	if (uDebugRenderType) {
		fragment.baseColor = uDebugRenderColor;
	}

#ifdef SET_DEPTH
	vec3 p = (viewMatrix * vec4(fragment.ws_coord, 1.0)).xyz;
	setFragmentDepth(p);
#endif // SET_DEPTH
    autoPackGFragment(fragment);
#endif // 0
}
