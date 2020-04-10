//////////////////////////////////////////////////////
// G-Buffer related functions
//
// define either OUT_GBUFFER or IN_GBUFFER before including this file,
// to respectivaly write or read to gbuffer.

struct GFragment {
	vec3 baseColor;
	vec3 normal;
	vec3 ws_coord;
	uint material_id;
	float roughness;
	float metallic;
	vec3 emission;
	float alpha;
	uint count;

	// Used for G-impostors but not in gbuffer (converted to regular normal)
	vec4 lean1;
	vec4 lean2;

};

//////////////////////////////////////////////////////
// Reserved material IDs

const uint noMaterial = 0;
const uint skyboxMaterial = 1;
const uint forwardAlbedoMaterial = 2;
const uint forwardBaseColorMaterial = 3;
const uint forwardNormalMaterial = 4;
const uint pbrMaterial = 5;
const uint pbrMetallicRoughnessMaterial = 6;
const uint iridescentMaterial = 7;
const uint worldMaterial = 8;
const uint iblMaterial = 9;
const uint farCloudTestMaterial = 10;
const uint colormapDebugMaterial = 11;
const uint accumulatedPbrMaterial = 12; // must always be last, cause with accumulation it is actually more than this value

//////////////////////////////////////////////////////
// GFragment methods

void initGFragment(out GFragment fragment) {
    fragment.baseColor = vec3(0.0);
    fragment.material_id = pbrMaterial;
    fragment.normal = vec3(0.0);
    fragment.ws_coord = vec3(0.0);
    fragment.roughness = 0.0;
    fragment.metallic = 0.0;
    fragment.emission = vec3(0.0);
    fragment.alpha = 0.0;
    fragment.count = 0;
    fragment.lean1 = vec4(0.0);
    fragment.lean2 = vec4(0.0);
}

/**
 * Apply mix to all components of a GFragment
 */
GFragment LerpGFragment(GFragment ga, GFragment gb, float t) {
	GFragment g;
	g.baseColor = mix(ga.baseColor, gb.baseColor, t);
	g.ws_coord = mix(ga.ws_coord, gb.ws_coord, t);
	g.metallic = mix(ga.metallic, gb.metallic, t);
	g.roughness = mix(ga.roughness, gb.roughness, t);
	g.emission = mix(ga.emission, gb.emission, t);
	g.alpha = mix(ga.alpha, gb.alpha, t);
	g.lean1 = mix(ga.lean1, gb.lean1, t);
	g.lean2 = mix(ga.lean2, gb.lean2, t);
	g.material_id = ga.material_id; // cannot be interpolated

	// Normal interpolation
	float wa = (1 - t) * ga.alpha;
	float wb = t * gb.alpha;

	// Avoid trouble with NaNs in normals
	if (wa == 0) ga.normal = vec3(0.0);
	if (wb == 0) gb.normal = vec3(0.0);

	g.normal = normalize(wa * ga.normal + wb * gb.normal);

	// Naive interpolation
	//g.normal = normalize(mix(ga.normal, gb.normal, t));
	return g;
}

void unpackGFragment(
	in sampler2D gbuffer0,
	in usampler2D gbuffer1,
	in usampler2D gbuffer2,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data1 = texelFetch(gbuffer0, coords, 0);
	uvec4 data2 = texelFetch(gbuffer1, coords, 0);
	uvec4 data3 = texelFetch(gbuffer2, coords, 0);
	vec2 tmp;

	tmp = unpackHalf2x16(data2.y);
	fragment.baseColor = vec3(unpackHalf2x16(data2.x), tmp.x);
	fragment.normal = normalize(vec3(tmp.y, unpackHalf2x16(data2.z)));
	fragment.ws_coord = data1.xyz;
	fragment.material_id = data2.w;
	fragment.roughness = data1.w;

	tmp = unpackHalf2x16(data3.y);
	fragment.emission = vec3(unpackHalf2x16(data3.x), tmp.x);
	fragment.metallic = tmp.y;
	fragment.count = data3.w;
}

void packGFragment(
	in GFragment fragment,
	out vec4 gbuffer_color0,
	out uvec4 gbuffer_color1,
	out uvec4 gbuffer_color2)
{
	gbuffer_color0 = vec4(fragment.ws_coord, fragment.roughness);
	gbuffer_color1 = uvec4(
		packHalf2x16(fragment.baseColor.xy),
		packHalf2x16(vec2(fragment.baseColor.z, fragment.normal.x)),
		packHalf2x16(fragment.normal.yz),
		fragment.material_id
	);
	gbuffer_color2 = uvec4(
		packHalf2x16(fragment.emission.xy),
		packHalf2x16(vec2(fragment.emission.z, fragment.metallic)),
		packHalf2x16(vec2(0.0, 0.0)),
		fragment.count
	);
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void unpackLinearGFragment(
	in sampler2D lgbuffer0,
	in sampler2D lgbuffer1,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data0 = texelFetch(lgbuffer0, coords, 0);
	vec4 data1 = texelFetch(lgbuffer1, coords, 0);
	fragment.baseColor.rgb = data0.rgb;
	fragment.alpha = data0.a;
	fragment.normal.xyz = data1.rgb;
	fragment.roughness = data1.a;
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void packLinearGFragment(
	in GFragment fragment,
	out vec4 lgbuffer_color0,
	out vec4 lgbuffer_color1)
{
	lgbuffer_color0 = vec4(fragment.baseColor.rgb, fragment.alpha);
	lgbuffer_color1 = vec4(fragment.normal.xyz, fragment.roughness);
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void unpackLeanLinearGFragment(
	in sampler2D lgbuffer0,
	in sampler2D lgbuffer1,
	in sampler2D lgbuffer2,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data0 = texelFetch(lgbuffer0, coords, 0);
	vec4 data1 = texelFetch(lgbuffer1, coords, 0);
	vec4 data2 = texelFetch(lgbuffer2, coords, 0);
	fragment.baseColor.rgb = data0.rgb;
	fragment.alpha = data0.a;
	fragment.lean1.xyz = data1.rgb;
	fragment.lean2.x = data1.a;
	fragment.lean2.yz = data2.rg;
	fragment.roughness = data2.b;
	fragment.metallic = data2.a;
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void packLeanLinearGFragment(
	in GFragment fragment,
	out vec4 lgbuffer_color0,
	out vec4 lgbuffer_color1,
	out vec4 lgbuffer_color2)
{
	lgbuffer_color0 = vec4(fragment.baseColor.rgb, fragment.alpha);
	lgbuffer_color1 = vec4(fragment.lean1.xyz, fragment.lean2.x);
	lgbuffer_color2 = vec4(fragment.lean2.yz, fragment.roughness, fragment.metallic);
}

/////////////////////////////////////////////////////////////////////
#ifdef OUT_GBUFFER

layout (location = 0) out vec4 gbuffer_color0;
layout (location = 1) out uvec4 gbuffer_color1;
layout (location = 2) out uvec4 gbuffer_color2;

void autoPackGFragment(in GFragment fragment) {
	packGFragment(fragment, gbuffer_color0, gbuffer_color1, gbuffer_color2);
}

#endif // OUT_GBUFFER


/////////////////////////////////////////////////////////////////////
#ifdef IN_GBUFFER

layout (binding = 0) uniform sampler2D gbuffer0;
layout (binding = 1) uniform usampler2D gbuffer1;
layout (binding = 2) uniform usampler2D gbuffer2;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackGFragment(gbuffer0, gbuffer1, gbuffer2, ivec2(gl_FragCoord.xy), fragment);
}

void autoUnpackGFragmentWithOffset(inout GFragment fragment, vec2 offset) {
	unpackGFragment(gbuffer0, gbuffer1, gbuffer2, ivec2(gl_FragCoord.xy - offset), fragment);
}

#endif // IN_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef OUT_LINEAR_GBUFFER

layout (location = 0) out vec4 lgbuffer_color0;
layout (location = 1) out vec4 lgbuffer_color1;

void autoPackGFragment(in GFragment fragment) {
	packLinearGFragment(fragment, lgbuffer_color0, lgbuffer_color1);
}

#endif // OUT_LINEAR_GBUFFER


/////////////////////////////////////////////////////////////////////
#ifdef IN_LINEAR_GBUFFER

layout (binding = 0) uniform sampler2D lgbuffer0;
layout (binding = 1) uniform sampler2D lgbuffer1;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackLinearGFragment(lgbuffer0, lgbuffer1, ivec2(gl_FragCoord.xy), fragment);
}

#endif // IN_LINEAR_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef OUT_LEAN_LINEAR_GBUFFER

layout (location = 0) out vec4 lgbuffer_color0;
layout (location = 1) out vec4 lgbuffer_color1;
layout (location = 2) out vec4 lgbuffer_color2;

void autoPackGFragment(in GFragment fragment) {
	packLeanLinearGFragment(fragment, lgbuffer_color0, lgbuffer_color1, lgbuffer_color2);
}

#endif // OUT_LEAN_LINEAR_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef IN_LEAN_LINEAR_GBUFFER

layout (binding = 0) uniform sampler2D lgbuffer0;
layout (binding = 1) uniform sampler2D lgbuffer1;
layout (binding = 2) uniform sampler2D lgbuffer2;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackLeanLinearGFragment(lgbuffer0, lgbuffer1, lgbuffer2, ivec2(gl_FragCoord.xy), fragment);
}

#endif // IN_LEAN_LINEAR_GBUFFER
