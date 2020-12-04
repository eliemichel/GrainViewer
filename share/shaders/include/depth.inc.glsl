//////////////////////////////////////////////////////
// Deapth related functions (require a camera ubo)

void setFragmentDepth(vec3 position_cs) {
	vec4 clipPos = projectionMatrix * vec4(position_cs, 1.0);
	float ndcDepth = clipPos.z / clipPos.w;
	gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
}
