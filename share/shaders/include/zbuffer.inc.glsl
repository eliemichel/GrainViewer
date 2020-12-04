//////////////////////////////////////////////////////
// Z-Buffer utils
// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

// CHange these uniforms if using glDepthRange()
uniform float uDepthRangeMin = 0.0f;
uniform float uDepthRangeMax = 1.0f;

float linearizeDepth(float logDepth) {
	float in01 = (logDepth - uDepthRangeMin) / (uDepthRangeMax - uDepthRangeMin);
	return (2.0 * uNear * uFar) / (uFar + uNear - (in01 * 2.0 - 1.0) * (uFar - uNear));
}

float unlinearizeDepth(float linearDepth) {
	//return (uFar + uNear - (2.0 * uNear * uFar) / linearDepth) / (uFar - uNear) * 0.5 + 0.5;
	float in01 = (uFar + uNear - (2.0 * uNear * uFar) / linearDepth) / (uFar - uNear) * 0.5 + 0.5;
	return uDepthRangeMin + in01 * (uDepthRangeMax - uDepthRangeMin);
}
