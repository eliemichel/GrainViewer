#version 450 core
#include "sys:defines"

#pragma variant WHITE_BACKGROUND TRANSPARENT_BACKGROUND OLD_BRDF SHOW_NORMAL SHOW_BASECOLOR SHOW_POSITION
#pragma variant HDR REINHART
//#define SHOW_POSITION
#define WHITE_BACKGROUND
#define HDR

#include "include/utils.inc.glsl"

in vec2 uv_coords;

layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D gbuffer1;
layout (binding = 1) uniform usampler2D gbuffer2;
layout (binding = 2) uniform usampler2D gbuffer3;
layout (binding = 3) uniform sampler2D in_depth;
layout (binding = 4) uniform samplerCubeArray filteredCubemaps;

uniform sampler2D iblBsdfLut;
uniform sampler3D iridescenceLut;

uniform mat4 viewMatrix;
uniform mat4 inverseViewMatrix;
uniform float time;

#include "include/light.inc.glsl"

uniform PointLight light[3];
uniform float lightPowerScale = 1.0;
uniform bool isShadowMapEnabled = true;

#include "include/gbuffer.inc.glsl"

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

void main() {
	GFragment fragment;
	unpackGFragment(gbuffer1, gbuffer2, gbuffer3, ivec2(gl_FragCoord.xy), fragment);

	OutputFragment out_fragment;
	switch (fragment.material_id) {
	case pbrMaterial:
	{
		vec3 camPos_ws = vec3(inverseViewMatrix[3]);
		vec3 toCam = normalize(camPos_ws - fragment.ws_coord);

		out_fragment.radiance = vec4(0.0);

		SurfaceAttributes surface;
		surface.baseColor = fragment.baseColor;
		surface.metallic = fragment.metallic;
		surface.roughness = fragment.roughness;
		surface.reflectance = 1.0;
		
		for (int k = 0 ; k < 3 ; ++k) {
			float shadow = 0;
			if (isShadowMapEnabled) {
				float shadowBias = shadowBiasFromNormal(light[k], fragment.normal);
				shadowBias = 0.000002; // hardcoded hack
				shadow = shadowAt(light[k], fragment.ws_coord, shadowBias);
			}
			
			vec3 toLight = normalize(light[k].position_ws - fragment.ws_coord);
			vec3 f = vec3(0.0);
#ifdef OLD_BRDF
			f = bsdfPbrMetallicRoughness(toCam, toLight, fragment.normal, surface.baseColor, surface.roughness, surface.metallic);
#else // OLD_BRDF
			f = brdf(toCam, fragment.normal, toLight, surface);
#endif // OLD_BRDF
			out_fragment.radiance += vec4(f, 1.0) * vec4(light[k].color * lightPowerScale, 1.0) * (1. - shadow);
		}
		out_fragment.radiance += vec4(fragment.emission, 0.0);
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
	default:
		out_fragment.radiance = vec4(fragment.baseColor, 1.0);
		break;
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

	// Minimap Shadow Depth
	if (false && gl_FragCoord.x < 512 && gl_FragCoord.y < 512) {
		float depth = texelFetch(light[0].shadowMap, ivec2(gl_FragCoord.xy * 2.0), 0).r;
		out_fragment.radiance = vec4(vec3(depth), 1.0);
	}
	//*/

#ifdef SHOW_NORMAL
	out_fragment.radiance.rgb = fragment.normal.xyz * .5 + .5;
#endif // SHOW_NORMAL
#ifdef SHOW_BASECOLOR
	out_fragment.radiance.rgb = fragment.baseColor.rgb;
#endif // SHOW_BASECOLOR
#ifdef SHOW_POSITION
	out_fragment.radiance.rgb = fragment.ws_coord.rgb;
#endif // SHOW_POSITION
#ifdef TRANSPARENT_BACKGROUND
	out_fragment.radiance.a = fragment.material_id == worldMaterial ? 0.0 : 1.0;
#endif // TRANSPARENT_BACKGROUND

	out_fragment.normal = fragment.normal;
	// TODO: compute from ws_coord and remove this extra buffer
	out_fragment.depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
	out_color = out_fragment.radiance;
}
