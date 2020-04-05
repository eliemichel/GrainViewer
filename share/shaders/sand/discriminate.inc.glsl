// require camera.inc.glsl

#include "../include/frustum.inc.glsl"

// Must match PointCloudSplitter::RenderModel
const uint cRenderModelInstance = 0;
const uint cRenderModelImpostor = 1;
const uint cRenderModelPoint = 2;
const uint cRenderModelNone = 3;

uniform bool uEnableOcclusionCulling = true;
uniform bool uEnableFrustumCulling = true;

uniform float uInstanceLimit = 1.05; // distance beyond which we switch from instances to impostors
uniform float uImpostorLimit = 10.0; // distance beyond which we switch from impostors to points

bool isInOcclusionCone(vec3 position_cs, vec3 otherGrain_cs, float innerRadius, float outerOverInnerRadius)
{
	vec3 closestCone_cs = otherGrain_cs * outerOverInnerRadius;
	float cosAlpha = dot(normalize(closestCone_cs), normalize(position_cs.xyz - closestCone_cs));
	if (cosAlpha >= 0 && closestCone_cs != vec3(0.0)) {
		float sinBeta = innerRadius / length(otherGrain_cs);
		float sin2Alpha = 1. - cosAlpha * cosAlpha;
		float sin2Beta = sinBeta * sinBeta;
		if (sin2Alpha < sin2Beta) {
			return true;
		}
	}
	return false;
}

/**
 * Choses the most appropriate model to render the point at model position
 * 'position', which may be no model at all if culling tests don't pass.
 * return one of cRenderModel* constants
 */
uint discriminate(
	vec3 position,
	float outerRadius,
	float innerRadius,
	float outerOverInnerRadius,
	sampler2D occlusionMap)
{
	vec4 position_cs = viewModelMatrix * vec4(position, 1.0);

	float instanceLimit2 = uInstanceLimit * uInstanceLimit;
	float impostorLimit2 = uImpostorLimit * uImpostorLimit;

	/////////////////////////////////////////
	// Distance-based discrimination
	uint model = cRenderModelNone;
	float l2 = dot(position_cs, position_cs);
	if (l2 < impostorLimit2) {
		model = l2 < instanceLimit2 ? cRenderModelInstance : cRenderModelImpostor;
	} else {
		model = cRenderModelPoint;
	}

	/////////////////////////////////////////
	// Frustum culling
	// unexplained multiplication factor...
	float fac = 4.0;
	if (uEnableFrustumCulling && SphereFrustumCulling(projectionMatrix, position_cs.xyz, fac*outerRadius)) {
		return cRenderModelNone;
	}

	/////////////////////////////////////////
	// Occlusion culling
	if (uEnableOcclusionCulling) {
		vec4 position_ps = projectionMatrix * vec4(position_cs.xyz, 1.0);
		vec2 fragCoord = resolution.xy * (position_ps.xy / position_ps.w * 0.5 + 0.5);

		fragCoord = clamp(fragCoord, vec2(0.5), resolution.xy - vec2(0.5));
		vec3 otherGrain_cs = texelFetch(occlusionMap, ivec2(fragCoord.xy), 0).xyz;
		if (isInOcclusionCone(position_cs.xyz, otherGrain_cs, innerRadius, outerOverInnerRadius)) {
			return cRenderModelNone;
		}

/*
		fragCoord += vec2(1, 0);
		fragCoord = clamp(fragCoord, vec2(0.5), resolution.xy - vec2(0.5));
		otherGrain_cs = texelFetch(occlusionMap, ivec2(fragCoord.xy), 0).xyz;
		if (isInOcclusionCone(position_cs.xyz, otherGrain_cs, innerRadius, outerOverInnerRadius)) {
			return cRenderModelNone;
		}

		fragCoord += vec2(-1, 1);
		fragCoord = clamp(fragCoord, vec2(0.5), resolution.xy - vec2(0.5));
		otherGrain_cs = texelFetch(occlusionMap, ivec2(fragCoord.xy), 0).xyz;
		if (isInOcclusionCone(position_cs.xyz, otherGrain_cs, innerRadius, outerOverInnerRadius)) {
			return cRenderModelNone;
		}
*/
	}

	return model;
}


