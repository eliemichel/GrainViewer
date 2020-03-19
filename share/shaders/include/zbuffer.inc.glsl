//////////////////////////////////////////////////////
// Z-Buffer utils
// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

float linearizeDepth(float logDepth) {
	return (2.0 * uNear * uFar) / (uFar + uNear - (logDepth * 2.0 - 1.0) * (uFar - uNear));
}

float unlinearizeDepth(float linearDepth) {
	return (uFar + uNear - (2.0 * uNear * uFar) / linearDepth) / (uFar - uNear) * 0.5 + 0.5;
}
