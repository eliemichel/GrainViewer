
struct StandardMaterial {
	sampler2D baseColorMap;
	sampler2D normalMap;
	sampler2D metallicRoughnessMap;
	sampler2D metallicMap;
	sampler2D roughnessMap;
	vec3 baseColor;
	float metallic;
	float roughness;
	bool hasBaseColorMap;
	bool hasNormalMap;
	bool hasMetallicRoughnessMap;
	bool hasMetallicMap;
	bool hasRoughnessMap;
};
