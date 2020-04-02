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
		position_ps = position_ps / position_ps.w * 0.5 + 0.5;
		position_ps.xy = clamp(position_ps.xy, vec2(0.0), vec2(1.0));
		vec3 otherGrain_cs = textureLod(occlusionMap, position_ps.xy, 0).xyz;
		vec3 closestCone_cs = otherGrain_cs * outerOverInnerRadius;
		float cosAlpha = dot(normalize(closestCone_cs), normalize(position_cs.xyz - closestCone_cs));
		if (cosAlpha >= 0 && closestCone_cs != vec3(0.0)) {
			float sinBeta = innerRadius / length(otherGrain_cs);
			float sin2Alpha = 1. - cosAlpha * cosAlpha;
			float sin2Beta = sinBeta * sinBeta;
			if (sin2Alpha < sin2Beta) {
				return cRenderModelNone;
			}
		}
	}

	return model;
}


