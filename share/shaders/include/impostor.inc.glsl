//////////////////////////////////////////////////////
// Light related functions
// requires gbuffer.inc.glsl and raytracing.inc.glsl

// requires that all impostors use the same number of views
#pragma opt PRECOMPUTE_IMPOSTOR_VIEW_MATRICES

#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
layout(std430, binding = 4) restrict readonly buffer impostorViewMatricesSsbo {
    mat4 impostorViewMatrices[];
};
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES

uniform float uGrainScale = 1.0;

struct SphericalImpostor {
	sampler2DArray normalAlphaTexture;
	sampler2DArray baseColorTexture;
	sampler2DArray metallicRoughnessTexture;
	sampler2DArray lean1Texture;
	sampler2DArray lean2Texture;
	uint viewCount; // number of precomputed views
	bool hasMetallicRoughnessMap;
	bool hasLeanMapping;
};

struct SphericalImpostorHit {
	vec3 position;
	vec3 textureCoords;
};

/**
 * Return only the index of the closest view to the direction
 */
uint DirectionToViewIndex(vec3 d, uint n) {
	d = d / dot(vec3(1, 1, 1), abs(d));
	vec2 uv = (vec2(1, -1) * d.y + d.x + 1) * (float(n) - 1) * 0.5;
	float i = dot(round(uv), vec2(n, 1));
	if (d.z > 0) {
		i += n * n;
	}
	return uint(i);
}

/**
 * Return the indices of the four closest views to the direction, as well as the
 * interpolation coefficients alpha.
 */
void DirectionToViewIndices(vec3 d, uint n, out uvec4 i, out vec2 alpha) {
	d = d / dot(vec3(1,1,1), abs(d));
	vec2 uv = (vec2(1, -1) * d.y + d.x + 1) * (n - 1) / 2;
	uvec2 fuv = uvec2(floor(uv)) * uvec2(n, 1);
	uvec2 cuv = uvec2(ceil(uv)) * uvec2(n, 1);
	i.x = fuv.x + fuv.y;
	i.y = cuv.x + fuv.y;
	i.z = fuv.x + cuv.y;
	i.w = cuv.x + cuv.y;
	if (d.z > 0) {
		i += n * n;
	}
	alpha = fract(uv);
}

/**
 * Return the direction of the i-th plane in an octahedric division of the unit
 * sphere of n subdivisions.
 */
vec3 ViewIndexToDirection(uint i, uint n) {
#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	mat4 m = impostorViewMatrices[i];
	return vec3(m[2].xyz);
#else // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	float eps = -1;
	uint n2 = n * n;
	if (i >= n2) {
		eps = 1;
		i -= n2;
	}
	vec2 uv = vec2(i / n, i % n) / float(n - 1);
	float x = uv.x + uv.y - 1;
	float y = uv.x - uv.y;
	float z = eps * (1. - abs(x) - abs(y));
	// break symmetry in redundant parts. TODO: find a mapping without redundancy, e.g. full octahedron
	if (z == 0) z = 0.0001 * eps;
	return normalize(vec3(x, y, z));
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
}

/**
 * Build the inverse of the view matrix used to bake the i-th G-Impostor
 */
mat4 InverseBakingViewMatrix(uint i, uint n) {
#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	return impostorViewMatrices[i];
#else // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	vec3 z = ViewIndexToDirection(i, n);
	vec3 x = normalize(cross(vec3(0, 0, 1), z));
	vec3 y = normalize(cross(z, x));
	vec3 w = vec3(0.0);
	return transpose(mat4(
		vec4(x, 0.0),
		vec4(y, 0.0),
		vec4(z, 0.0),
		vec4(w, 1.0)
	));
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
}

/**
 * Return the position and texture coordinates intersected by the ray in the i-th G-billboard of a spherical G-impostor
 * Basic "PlaneHit" sampling scheme
 */
SphericalImpostorHit IntersectRayBillboard(Ray ray, uint i, float radius, uint n) {
	mat4 bs_from_ws = InverseBakingViewMatrix(i, n);
	Ray ray_bs = TransformRay(ray, bs_from_ws);

	float l = -(ray_bs.origin.z / ray_bs.direction.z);
	vec3 uv_bs = ray_bs.origin + l * ray_bs.direction;
	vec2 uv_ts = uv_bs.xy / radius * .5 + .5;
	uv_ts.y = 1.0 - uv_ts.y;

	SphericalImpostorHit hit;
	hit.position = ray.origin + l * ray.direction;
	hit.textureCoords = vec3(clamp(uv_ts, vec2(0.0), vec2(1.0)), i);
	return hit;
}

/**
 * "SphereHit" sampling scheme
 */
SphericalImpostorHit IntersectRayBillboard_SphereHit(Ray ray, uint i, float radius, uint n, float hitSphereCorrectionFactor) {
	vec3 hitPosition;
	if (!intersectRaySphere(hitPosition, ray, vec3(0.0), radius * hitSphereCorrectionFactor)) {
		SphericalImpostorHit hit;
		hit.position = vec3(-1.0);
		hit.textureCoords = vec3(-1);
		return hit; // No hit
	}
	ray.origin = hitPosition;
	ray.direction = -ViewIndexToDirection(i, n);
	
	return IntersectRayBillboard(ray, i, radius, n);
}

/**
 * Mix of "PlaneHit" and "SphereHit" sampling schemes
 */
SphericalImpostorHit IntersectRayBillboard_MixedHit(Ray ray, uint i, float radius, uint n, float hitSphereCorrectionFactor) {
	SphericalImpostorHit sphereHit = IntersectRayBillboard_SphereHit(ray, i, radius, n, hitSphereCorrectionFactor);
	SphericalImpostorHit planeHit = IntersectRayBillboard(ray, i, radius, n);

	if (sphereHit.textureCoords.x < 0) {
		return planeHit;
	}

	float distanceToCenter = length(ray.origin - dot(ray.origin, ray.direction) / dot(ray.direction,ray.direction) * ray.direction) / radius;
	float t = smoothstep(hitSphereCorrectionFactor * .2, hitSphereCorrectionFactor, distanceToCenter);

	SphericalImpostorHit hit;
	hit.position = mix(sphereHit.position, planeHit.position, t);
	hit.textureCoords = mix(sphereHit.textureCoords, planeHit.textureCoords, t);
	return hit;
}

/**
 * Sample the billboard textures to return a GFragment
 */
GFragment SampleBillboard(SphericalImpostor impostor, SphericalImpostorHit hit) {
	// If invalid hit, return transparent fragment
	if (hit.textureCoords.x <= 0 || hit.textureCoords.x >= 1 || hit.textureCoords.y <= 0 || hit.textureCoords.y >= 1) {
		GFragment g;
		g.alpha = 0.0;
		return g;
	}
	
	vec3 uvw = vec3((hit.textureCoords.xy - 0.5) * uGrainScale + 0.5, hit.textureCoords.z);

	// Otherwise, sample textures
	vec4 normalAlpha = texture(impostor.normalAlphaTexture, uvw);
	vec4 lean1 = vec4(0.0);
	vec4 lean2 = vec4(0.0);
	vec4 baseColor = vec4(0.0);
	vec4 metallicRoughnes = vec4(0.0);
	if (normalAlpha.a > 0) {
		baseColor = texture(impostor.baseColorTexture, uvw);
		lean1 = texture(impostor.lean1Texture, uvw);
		lean2 = texture(impostor.lean2Texture, uvw);
		metallicRoughnes = texture(impostor.metallicRoughnessTexture, uvw);
	}

	GFragment g;
	g.baseColor = baseColor.rgb;
	g.normal = normalAlpha.xyz * 2. - 1.;
	g.lean1 = lean1;
	g.lean2 = lean2;
	g.ws_coord = hit.position;
	g.metallic = metallicRoughnes.x;
	g.roughness = metallicRoughnes.y;
	//g.emission = __;
	g.alpha = normalAlpha.a;
	return g;
}

/**
 * Sample a Spherical G-impostor of radius radius and located at world position p
 */
GFragment IntersectRaySphericalGBillboard(SphericalImpostor impostor, Ray ray, float radius) {
	uint n = impostor.viewCount;
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, n, i, alpha);

	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.x, radius, n));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.y, radius, n));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.z, radius, n));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.w, radius, n));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}
GFragment IntersectRaySphericalGBillboard_SphereHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor) {
	uint n = impostor.viewCount;
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, n, i, alpha);

	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.x, radius, n, hitSphereCorrectionFactor));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.y, radius, n, hitSphereCorrectionFactor));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.z, radius, n, hitSphereCorrectionFactor));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.w, radius, n, hitSphereCorrectionFactor));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}
GFragment IntersectRaySphericalGBillboard_MixedHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor) {
	uint n = impostor.viewCount;
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, n, i, alpha);

	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.x, radius, n, hitSphereCorrectionFactor));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.y, radius, n, hitSphereCorrectionFactor));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.z, radius, n, hitSphereCorrectionFactor));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.w, radius, n, hitSphereCorrectionFactor));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}

/**
 * Same as IntersectRaySphericalGBillboard wthout interpolation
 */
GFragment IntersectRaySphericalGBillboardNoInterp(SphericalImpostor impostor, Ray ray, float radius) {
	uint n = impostor.viewCount;
	uint i = DirectionToViewIndex(-ray.direction, n);
	SphericalImpostorHit hit = IntersectRayBillboard(ray, i, radius, n);
	//SphericalImpostorHit hit = SphericalImpostorHit(vec3(0.0), vec3(0.0));
	GFragment g = SampleBillboard(impostor, hit);
	//GFragment g; initGFragment(g); g.normal = hit.position;
	return g;
}

/**
 * Intersect with sphere (for debug)
 */
GFragment IntersectRaySphere(Ray ray, vec3 p, float radius) {
	GFragment g;
	g.material_id = forwardBaseColorMaterial;

	vec3 hitPosition;
	if (intersectRaySphere(hitPosition, ray, p, radius)) {
		g.baseColor.rgb = normalize(hitPosition - p) * .5 + .5;
		g.normal.rgb = normalize(hitPosition - p);
		g.alpha = 1.0;
	} else {
		g.alpha = 0.0;
	}

	return g;
}

/**
 * Intersect with cube (for debug)
 */
GFragment IntersectRayCube(Ray ray, vec3 p, float radius) {
	GFragment g;
	g.material_id = forwardBaseColorMaterial;

	float minl = 9999999.9;
	g.alpha = 0.0;
	for (int j = 0; j < 6; ++j) {
		vec3 n;
		if (j < 2) n = vec3((j % 2) * 2 - 1, 0, 0);
		else if (j < 4) n = vec3(0, (j % 2) * 2 - 1, 0);
		else n = vec3(0, 0, (j % 2) * 2 - 1);
		vec3 pp = p + radius * n;
		vec3 dp = pp - ray.origin;
		float l = dot(dp, n) / dot(ray.direction, n);
		if (l > 0 && l < minl) {
			vec3 i = ray.origin + l * ray.direction;

			vec3 ii = i - pp;
			float linf = max(max(abs(ii.x), abs(ii.y)), abs(ii.z));
			if (linf <= radius) {
				g.baseColor.rgb = n * .5 + .5;
				g.alpha = 1.0;
				g.normal = n;
				minl = l;
			}
		}
	}

	return g;
}
