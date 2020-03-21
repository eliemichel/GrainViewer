// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

/**
 * Estimate projection of sphere on screen to determine sprite size (diameter).
 * (TODO: actually, this is the diameter...)
 */
float SpriteSize(float radius, vec4 position_clipspace) {
    if (isOrthographic(projectionMatrix)) {
        float a = projectionMatrix[0][0] * resolution.x;
        float c = projectionMatrix[1][1] * resolution.y;
        return max(a, c) * radius;
    } else {
        return max(resolution.x, resolution.y) * projectionMatrix[1][1] * radius / position_clipspace.w;
    }
}

float SpriteSize_Botsch03(float radius, vec4 position_cameraspace) {
    if (isOrthographic(projectionMatrix)) {
        return 2.0 * radius * resolution.y / (uTop - uBottom);
    }
	return 2.0 * radius * uNear / abs(position_cameraspace.z) * resolution.y / (uTop - uBottom);
}

/**
 * Corrects to take into account the fact that projected bounding spheres are
 * ellipses, not circles. Returns ellipse's largest radius.
 */
float SpriteSize_Botsch03_corrected(float radius, vec4 position_cameraspace) {
	if (isOrthographic(projectionMatrix)) {
        return 2.0 * radius * resolution.y / (uTop - uBottom);
    }
	float l2 = dot(position_cameraspace.xyz, position_cameraspace.xyz);
	float z = position_cameraspace.z;
	float z2 = z * z;
	float r2 = radius * radius;
	return 2.0 * radius * uNear * sqrt(l2 - r2) / abs(r2 - z2) * resolution.y / (uTop - uBottom);
}
