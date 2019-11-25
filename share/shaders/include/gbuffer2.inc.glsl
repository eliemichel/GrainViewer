//////////////////////////////////////////////////////
// G-Buffer related functions
//
// This version si newer than gbuffer.inc.glsl and intended to replace the latter

struct GFragment {
	vec3 baseColor;
	vec3 normal;
	vec3 ws_coord;
	uint material_id;
	float roughness;
	float metallic;
	vec3 emission;
};

void unpackGFragment(
	in sampler2D gbuffer1,
	in usampler2D gbuffer2,
	in usampler2D gbuffer3,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data1 = texelFetch(gbuffer1, coords, 0);
	uvec4 data2 = texelFetch(gbuffer2, coords, 0);
	uvec4 data3 = texelFetch(gbuffer3, coords, 0);
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
}

void packGFragment(
	in GFragment fragment,
	out vec4 gbuffer_color1,
	out uvec4 gbuffer_color2,
	out uvec4 gbuffer_color3)
{
	gbuffer_color1 = vec4(fragment.ws_coord, fragment.roughness);
	gbuffer_color2 = uvec4(
		packHalf2x16(fragment.baseColor.xy),
		packHalf2x16(vec2(fragment.baseColor.z, fragment.normal.x)),
		packHalf2x16(fragment.normal.yz),
		fragment.material_id
	);
	gbuffer_color3 = uvec4(
		packHalf2x16(fragment.emission.xy),
		packHalf2x16(vec2(fragment.emission.z, fragment.metallic)),
		packHalf2x16(vec2(0.0, 0.0)),
		0
	);
}


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
