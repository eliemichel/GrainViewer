#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR SET_DEPTH
#pragma hidden_variant SHADOW_MAP
// if both MIXEDHIT_SAMPLING and SPHEREHIT_SAMPLING are defined, sampling will be mixed.
// NO_INTERPOLATION is defined only for default sampling (planeHit)
// SET_DEPTH allows for self intersection but significantly degrades performances

//#define MIXEDHIT_SAMPLING
//#define SPHEREHIT_SAMPLING

flat in uint id;
in float radius;
in vec3 position_ws;
in mat4 gs_from_ws;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;
#ifdef SET_DEPTH
layout (depth_less) out float gl_FragDepth;
#endif // SET_DEPTH

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/gbuffer.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/impostor.inc.glsl"
#include "include/random.inc.glsl"
#include "include/depth.inc.glsl"
#include "sand/random-grains.inc.glsl"

uniform SphericalImpostor impostor[3];
uniform sampler2D colormapTexture;
uniform float uInnerRadius;

uniform float hitSphereCorrectionFactor = .65;//0.65;

uniform bool uHasMetallicRoughnessMap = false;
uniform float uDefaultRoughness = 0.5;
uniform float uDefaultMetallic = 0.0;

// Global switches
uniform int uDebugShape = -1; // 0: DEBUG_SPHERE 1: DEBUG_INNER_SPHERE 2: DEBUG_CUBE
uniform int uInterpolationMode = 1; // 0: NO_INTERPOLATION 1: INTERPOLATION
uniform int uSamplingMode = 0; // 0: PLANEHIT_SAMPLING 1: SPHEREHIT_SAMPLING 2: MIXEDHIT_SAMPLING

void main() {
	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
	vec3 innerSphereHitPosition_ws;

#ifdef SHADOW_MAP
#ifndef SET_DEPTH
	// Early quit for shadow maps unless we alter fragment depth
	bool hit = intersectRaySphere(innerSphereHitPosition_ws, ray_ws, position_ws.xyz, radius*2);
	if (!hit) {
		discard;
	}
	return;
#endif
#endif

	bool intersectInnerSphere = intersectRaySphere(innerSphereHitPosition_ws, ray_ws, position_ws.xyz, uInnerRadius);
	vec3 outerSphereHitPosition_ws;
	intersectRaySphere(outerSphereHitPosition_ws, ray_ws, position_ws.xyz, radius);

	mat3 ws_from_gs_rot = transpose(mat3(gs_from_ws));

    Ray ray_gs = TransformRay(ray_ws, gs_from_ws);

    vec3 position_gs = (gs_from_ws * vec4(position_ws, 1.0)).xyz;

    GFragment fragment;
	if (uInterpolationMode == 0) {
		fragment = IntersectRaySphericalGBillboardNoInterp(impostor[0], ray_gs, position_gs, radius);
	} else {
		switch (uSamplingMode) {
		case 0: // PLANEHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard(impostor[0], ray_gs, position_gs, radius);
			break;
		case 1: // SPHEREHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard_SphereHit(impostor[0], ray_gs, position_gs, radius, hitSphereCorrectionFactor);
			break;
		case 2: // MIXEDHIT_SAMPLING
			fragment = IntersectRaySphericalGBillboard_MixedHit(impostor[0], ray_gs, position_gs, radius, hitSphereCorrectionFactor);
			break;
		}
	}
	
	switch (uDebugShape) {
	case 0: // DEBUG_SPHERE
		fragment = IntersectRaySphere(ray_ws, position_ws, radius);
		break;
	case 1: // DEBUG_INNER_SPHERE
		fragment = IntersectRaySphere(ray_ws, position_ws, uInnerRadius);
		break;
	case 2: // DEBUG_CUBE
		fragment = IntersectRayCube(ray_ws, position_ws, radius);
		break;
	default:
		fragment.normal = ws_from_gs_rot * fragment.normal;
		break;
	}

	//fragment.ws_coord = ws_from_gs_rot * (fragment.ws_coord - position_gs) + position_ws;
	//fragment.ws_coord = innerSphereHitPosition_ws;
	fragment.ws_coord = outerSphereHitPosition_ws;
	fragment.material_id = pbrMaterial;

	if (fragment.alpha < 0.5) discard;
	//if (dot(fragment.normal, ray_ws.direction) >= 0.0) discard;

#ifdef PROCEDURAL_BASECOLOR
    float r = randomGrainColorFactor(int(id));
    fragment.baseColor = texture(colormapTexture, vec2(r, 0.0)).rgb;
#endif // PROCEDURAL_BASECOLOR
#ifdef PROCEDURAL_BASECOLOR2
	float r0 = randomGrainColorFactor(int(id));
	float r = position_ws.z*.6 + mix(0.35, 0.3, sin(position_ws.x*.5+.5))*r0;
	if (r0 < 0.5) {
		r = 1. - r0;
	}
    fragment.baseColor = texture(colormapTexture, vec2(r, 0.0)).rgb;
    fragment.baseColor *= mix(vec3(0.9, 0.9, 0.9), vec3(1.6, 2.0, 2.0), r0);
#endif // PROCEDURAL_BASECOLOR2

	if (!uHasMetallicRoughnessMap) {
		fragment.metallic = uDefaultMetallic;
		fragment.roughness = uDefaultRoughness;
	}

	fragment.baseColor *= 1.0;

#ifdef SET_DEPTH
	vec3 p = (viewMatrix * vec4(fragment.ws_coord, 1.0)).xyz;
	setFragmentDepth(p);
#endif // SET_DEPTH
    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}
