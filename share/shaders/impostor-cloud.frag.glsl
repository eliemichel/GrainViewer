#version 450 core
#include "sys:defines"

#pragma variant NO_INTERPOLATION PROCEDURAL_BASECOLOR DEBUG_SPHERE DEBUG_INNER_SPHERE DEBUG_CUBE SET_DEPTH
#pragma variant SPHEREHIT_SAMPLING MIXEDHIT_SAMPLING
// if both MIXEDHIT_SAMPLING and SPHEREHIT_SAMPLING are defined, sampling will be mixed.
// NO_INTERPOLATION is defined only for default sampling (planeHit)

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

uniform float hitSphereCorrectionFactor = 0.65;
uniform float roughness = 0.5;
uniform float metallic = 0.0;

// DEBUG
uniform sampler2D occlusionMap;

void main() {
	mat3 ws_from_gs_rot = transpose(mat3(gs_from_ws));

	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
    Ray ray_gs = TransformRay(ray_ws, gs_from_ws);

    vec3 position_gs = (gs_from_ws * vec4(position_ws, 1.0)).xyz;

#ifdef NO_INTERPOLATION
	GFragment fragment = IntersectRaySphericalGBillboardNoInterp(impostor[0], ray_gs, position_gs, radius);
#elif MIXEDHIT_SAMPLING
	GFragment fragment = IntersectRaySphericalGBillboard_MixedHit(impostor[0], ray_gs, position_gs, radius, hitSphereCorrectionFactor);
#elif SPHEREHIT_SAMPLING
	GFragment fragment = IntersectRaySphericalGBillboard_SphereHit(impostor[0], ray_gs, position_gs, radius, hitSphereCorrectionFactor);
#else
	GFragment fragment = IntersectRaySphericalGBillboard(impostor[0], ray_gs, position_gs, radius);
#endif
	
#ifdef DEBUG_SPHERE
	fragment = IntersectRaySphere(ray_ws, position_ws, radius);
#endif
#ifdef DEBUG_INNER_SPHERE
	fragment = IntersectRaySphere(ray_ws, position_ws, uInnerRadius);
#endif
#ifdef DEBUG_CUBE
	fragment = IntersectRayCube(ray_ws, position_ws, radius);
#endif

	fragment.normal = ws_from_gs_rot * fragment.normal;
	fragment.ws_coord = ws_from_gs_rot * (fragment.ws_coord - position_gs) + position_ws;
	fragment.material_id = pbrMaterial;

	if (fragment.alpha < 0.2) discard;
	//if (dot(fragment.normal, ray_ws.direction) >= 0.0) discard;

#ifdef PROCEDURAL_BASECOLOR
	float r = randomGrainColorFactor(int(id));
    fragment.baseColor = texture(colormapTexture, vec2(r, 0.0)).rgb;
#endif

	fragment.metallic = metallic;
	fragment.roughness = roughness;

#ifdef SET_DEPTH
	vec3 p = (viewMatrix * vec4(fragment.ws_coord, 1.0)).xyz;
	setFragmentDepth(p);
#endif // SET_DEPTH
    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}
