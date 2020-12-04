// Requires gbuffer.inc.glsl

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

struct SurfacePoint {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    float normal_mapping;
};

GFragment SampleStandardMaterial(const StandardMaterial mat, const SurfacePoint geo) {
	GFragment fragment;

    // Normal
    vec3 normal = geo.normal_ws;
    if (geo.normal_mapping > 0 && mat.hasNormalMap) {
        vec3 normal_ts = vec3(texture(mat.normalMap, geo.uv)) * 2.0 - 1.0;
        // Normal mapping
        mat3 TBN = mat3(
            normalize(-geo.tangent_ws),
            normalize(-cross(geo.normal_ws, geo.tangent_ws)),
            normalize(geo.normal_ws)
        );
        normal = geo.normal_ws + TBN * normal_ts * geo.normal_mapping;
    }
    fragment.normal = normalize(normal);

    // Base Color
    if (mat.hasBaseColorMap) {
        fragment.baseColor = texture(mat.baseColorMap, geo.uv).rgb;
    } else {
        fragment.baseColor = mat.baseColor;
    }

    // Metallic/Roughness
    if (mat.hasMetallicRoughnessMap) {
        vec4 t = texture(mat.metallicRoughnessMap, geo.uv);
        fragment.metallic = t.x;
        fragment.roughness = t.y;
    } else {
        if (mat.hasMetallicMap) {
            fragment.metallic = texture(mat.metallicMap, geo.uv).r;
        } else {
            fragment.metallic = mat.metallic;
        }
        if (mat.hasRoughnessMap) {
            fragment.roughness = texture(mat.roughnessMap, geo.uv).r;
        } else {
            fragment.roughness = mat.roughness;
        }
    }
    
    // Other attributes
    fragment.ws_coord = geo.position_ws;
    fragment.material_id = pbrMaterial;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;
    return fragment;
}
