//////////////////////////////////////////////////////
// LEAN-mapping related functions
//

struct LeanStatistics {
	vec2 B;
	vec3 sigma;
	float Det;
};

/**
 * Call this on level 0 of mip maps (then generate other mip maps for lean
 * maps in a standard way).
 *  tn: input normal map in tangent space
 *  t1, t2: packed lean statistics
 *  s: scale parameter from eq 5
 *  sc: scale parameter to fit packing in high definition range
 */
void normalToLeanMaps(const in vec3 tn, out vec4 t1, out vec4 t2, float s, float sc) {
	vec3 N = vec3(2.0 * tn.xy - 1.0, tn.z);

	vec2 B = N.xy / (sc * N.z);
	vec3 M = vec3(B.x * B.x + 1.0 / s, B.y * B.y + 1.0 / s, B.x * B.y);

	t1 = vec4(tn, M.z * 0.5 + 0.5);
	t2 = vec4(B * 0.5 + 0.5, M.xy);
}

LeanStatistics unpackLeanStatistics(const vec4 t1, const vec4 t2, float sc) {
	// unpack normal
	vec3 N = vec3(2.0 * t1.xy - 1.0, t1.z);

	// unpack B and M
	vec2 B = (2.0 * t2.xy - 1.0) * sc;
	vec3 M = vec3(t2.zw, 2.0 * t1.w - 1.0) * sc * sc;

	// convert M to sigma
	vec3 sigma = M - vec3(B * B, B.x * B.y);
	float Det = sigma.x * sigma.y - sigma.z * sigma.z;

	return LeanStatistics(B, sigma, Det);
}

/**
 * Get specular from half-vector
 */
float leanSpecular(const LeanStatistics stats, const vec3 h) {
	if (stats.Det <= 0.0) return 0.0;

	// compute specular
	vec2 h_bar = h.xy / h.z - stats.B;
	float e = (
		  h_bar.x * h_bar.x * stats.sigma.y
		+ h_bar.y * h_bar.y * stats.sigma.x
		- 2.0 * h_bar.x * h_bar.y * stats.sigma.z
	);
	return exp(-0.5 * e / stats.Det) / sqrt(stats.Det);
}
