// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

/**
 * Estimate projection of sphere on screen to determine sprite size.
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
	return radius * uNear / abs(position_cameraspace.z) * resolution.y / (uTop - uBottom);
}

/**
 *
 */
float SpriteSize_Botsch03_corrected(float radius, vec4 position_cameraspace) {
	if (isOrthographic(projectionMatrix)) {
        return SpriteSize_Botsch03(radius, position_cameraspace);
    }
	float l2 = dot(position_cameraspace.xyz, position_cameraspace.xyz);
	float z = position_cameraspace.z;
	float z2 = z * z;
	float r2 = radius * radius;
	return radius * uNear * sqrt(l2 - r2) / abs(r2 - z2) * resolution.y / (uTop - uBottom);
}
