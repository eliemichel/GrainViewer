// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

/**
 * Estimate projection of sphere on screen to determine sprite size
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
