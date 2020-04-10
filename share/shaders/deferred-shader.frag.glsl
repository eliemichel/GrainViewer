#version 450 core
#include "sys:defines"

#pragma variant WHITE_BACKGROUND TRANSPARENT_BACKGROUND OLD_BRDF
#pragma variant SHOW_NORMAL SHOW_BASECOLOR SHOW_POSITION SHOW_RAW_BUFFER1 SHOW_ROUGHNESS
#pragma variant HDR REINHART

#define WHITE_BACKGROUND
#define HDR

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"

in vec2 uv_coords;

layout (location = 0) out vec4 out_color;

#define IN_GBUFFER
#include "include/gbuffer2.inc.glsl"

layout (binding = 7) uniform sampler2D in_depth;
layout (binding = 8) uniform samplerCubeArray filteredCubemaps;

uniform sampler2D iblBsdfLut;
uniform sampler3D iridescenceLut;

uniform float uTime;

uniform sampler2D uColormap;
uniform bool uHasColormap = false;

#include "include/light.inc.glsl"

uniform PointLight light[3];
uniform float lightPowerScale = 1.0;

// Global switches
uniform bool uIsShadowMapEnabled = true;
uniform bool uTransparentFilm = false;
uniform int uShadingMode = 0; // 0: BEAUTY, 1: NORMAL, 2: BASE_COLOR

// Accumulated samples info display
uniform float uMaxSampleCount = 40;
uniform bool uShowSampleCount = false;

uniform float uShadowMapBias;
uniform vec2 uBlitOffset = vec2(0.0);


#ifdef OLD_BRDF
#include "include/bsdf-old.inc.glsl"
#else
#include "include/bsdf.inc.glsl"
#endif

struct OutputFragment {
	vec4 radiance;
	vec3 normal;
	float depth;
};

void pbrShading(const in GFragment fragment, out OutputFragment out_fragment)
{
	vec3 camPos_ws = vec3(inverseViewMatrix[3]);
	vec3 toCam = normalize(camPos_ws - fragment.ws_coord);

	SurfaceAttributes surface;
	surface.baseColor = fragment.baseColor;
	surface.metallic = fragment.metallic;
	surface.roughness = fragment.roughness;
	surface.reflectance = 0.7;

	out_fragment.radiance = vec4(0.0, 0.0, 0.0, 1.0);

	for (int k = 0 ; k < 3 ; ++k) {
		float shadow = 0;
		if (uIsShadowMapEnabled) {
			float shadowBias = shadowBiasFromNormal(light[k], fragment.normal);
			shadowBias = 0.0001; // hardcoded hack
			//shadowBias = 0.00005;
			shadow = shadowAt(light[k], fragment.ws_coord, uShadowMapBias);
			shadow = clamp(shadow, 0.0, 1.0);
			//shadow *= .8;
		}
		
		vec3 toLight = normalize(light[k].position_ws - fragment.ws_coord);
		vec3 f = vec3(0.0);
#ifdef OLD_BRDF
		f = bsdfPbrMetallicRoughness(toCam, toLight, fragment.normal, surface.baseColor, surface.roughness, surface.metallic);
#else // OLD_BRDF
		f = brdf(toCam, fragment.normal, toLight, surface);
#endif // OLD_BRDF
		out_fragment.radiance.rgb += f * light[k].color * lightPowerScale * (1. - shadow);
	}
	out_fragment.radiance += vec4(fragment.emission, 0.0);
}

void main() {
	GFragment fragment;
	autoUnpackGFragmentWithOffset(fragment, uBlitOffset);

	OutputFragment out_fragment;
	switch (fragment.material_id) {
	case pbrMaterial:
	{
		pbrShading(fragment, out_fragment);
		break;
	}

	case forwardBaseColorMaterial:
	case forwardAlbedoMaterial:
		out_fragment.radiance = vec4(fragment.baseColor, 1.0);
		break;

	case forwardNormalMaterial:
		out_fragment.radiance = normal2color(fragment.normal, 1.0);
		break;

	case worldMaterial:
	{
#ifdef WHITE_BACKGROUND
		out_fragment.radiance = vec4(1.0);
#else // WHITE_BACKGROUND
		vec3 camPos_ws = vec3(inverseViewMatrix[3]);
		vec3 toCam = normalize(camPos_ws - fragment.ws_coord);
		out_fragment.radiance = sampleSkybox(-toCam);
#endif // WHITE_BACKGROUND
		break;
	}

	case skyboxMaterial:
		out_fragment.radiance = vec4(fragment.baseColor, 1.0);
		break;

	case colormapDebugMaterial:
	{
		float sampleCount = fragment.baseColor.r;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(sampleCount / uMaxSampleCount, 0.0, 1.0), 0.5), 0).rgb;
		} else {
			out_fragment.radiance.rgb = vec3(sampleCount / uMaxSampleCount);
		}
	}

	default:
		if (fragment.material_id >= accumulatedPbrMaterial) {
			float sampleCount = fragment.roughness;
			float oneOverSampleCount = 1.0 / sampleCount;
			//GFragment normalizedFragment = fragment;
			//normalizedFragment.baseColor *= oneOverSampleCount;
			//normalizedFragment.roughness *= oneOverSampleCount;
			//normalizedFragment.metallic *= oneOverSampleCount;
			//normalizedFragment.ws_coord *= oneOverSampleCount;
			//normalizedFragment.normal *= oneOverSampleCount;
			//pbrShading(fragment, out_fragment);
			out_fragment.radiance.rgb = fragment.ws_coord * oneOverSampleCount;
			if (uShowSampleCount) {
				if (uHasColormap) {
					out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(sampleCount / uMaxSampleCount, 0.0, 1.0), 0.5), 0).rgb;
				} else {
					out_fragment.radiance.rgb = vec3(sampleCount / uMaxSampleCount);
				}
			}
		}
	}
	//gl_FragDepth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;

	// Tone mapping
#ifdef HDR
// TODO: look at filmic tone mapping
#ifdef REINHART
	out_fragment.radiance = out_fragment.radiance / (out_fragment.radiance + 1.0);
#else
	const float exposure = 3.5;
	out_fragment.radiance = 1.0 - exp(-out_fragment.radiance * exposure);
#endif
	const float gamma = 0.8;
	out_fragment.radiance = pow(out_fragment.radiance, vec4(1.0 / gamma));
#endif

	/*/ Minimap Shadow Depth
	if (gl_FragCoord.x < 256 && gl_FragCoord.y < 256) {
		float depth = texelFetch(light[0].shadowMap, ivec2(gl_FragCoord.xy * 4.0), 0).r;
		out_fragment.radiance = vec4(vec3(pow(1. - depth, 0.1)), 1.0);
	}
	//*/

	switch (uShadingMode) {
	case 0: // BEAUTY
		break;
	case 1: // NORMAL
		out_fragment.radiance.rgb = normalize(fragment.normal) * .5 + .5;
		break;
	case 2: // BASE_COLOR
		out_fragment.radiance.rgb = fragment.baseColor.rgb;
		break;
	case 3: // METALLIC
		out_fragment.radiance.rgb = vec3(fragment.metallic);
		out_fragment.radiance.a = 1.0;
		break;
	case 4: // ROUGHNESS
		out_fragment.radiance.rgb = vec3(fragment.roughness);
		out_fragment.radiance.a = 1.0;
		break;
	case 5: // WORLD_POSITION
		out_fragment.radiance.rgb = fragment.ws_coord.rgb;
		break;
	case 6: // DEPTH
	{
		float depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
		float linearDepth = (2.0 * uNear * uFar) / (uFar + uNear - (depth * 2.0 - 1.0) * (uFar - uNear));
		out_fragment.radiance.rgb = vec3(linearDepth / uFar);
		break;
	}
	case 7: // RAW_GBUFFER1
		out_fragment.radiance.rgb = texelFetch(gbuffer0, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		break;
	case 8: // RAW_GBUFFER2
		out_fragment.radiance.rgb = texelFetch(gbuffer1, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		//out_fragment.radiance.rgb = 0.5 + 0.5 * cos(2.*3.1416 * (clamp(1.-out_fragment.radiance.r, 0.0, 1.0) * .5 + vec3(.0,.33,.67)));
		break;
	case 9: // RAW_GBUFFER3
		out_fragment.radiance.rgb = texelFetch(gbuffer2, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		break;
	}

	if (uTransparentFilm) {
		if (fragment.material_id == worldMaterial) {
			out_fragment.radiance = vec4(1.0, 1.0, 1.0, 0.0);
		} else {
			out_fragment.radiance.a = 1.0;
		}
	}

	out_fragment.normal = fragment.normal;
	out_fragment.radiance.a = 1.0;
	// out_fragment was designed to be sent to post effects, ne need here
	//out_fragment.depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
	out_color = out_fragment.radiance;
}
