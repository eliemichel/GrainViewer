/**
 * Parameter 'planes' contains coefficients (a, b, c, d) such that (x,y,z) is a point of the plane iff ax+by+cz+d=0
 * There are six frustum planes, in this order: left, right, top, bottom, near, far
 * See http://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf for explaination
 */
void ExtractFrustumPlanes(const mat4 projectionMatrix, out vec4 planes[6]) {
	mat4 m = transpose(projectionMatrix);
	planes[0] = m[3] + m[0];
	planes[1] = m[3] - m[0]; 
	planes[2] = m[3] + m[1];
	planes[3] = m[3] - m[1];
	planes[4] = m[3] + m[2];
	planes[5] = m[3] - m[2];
}

/**
 * Frustum culling of a sphere at position p of radius r
 */
bool SphereFrustumCulling(const mat4 projectionMatrix, vec3 p, float radius) {
	vec4 planes[6];
	ExtractFrustumPlanes(projectionMatrix, planes);
	for (int i = 0; i < 5; i++) {
		float dist = dot(vec4(p, 1.0), planes[i]);
		if (dist < -radius) return true; // sphere culled
	}
	return false;
}
