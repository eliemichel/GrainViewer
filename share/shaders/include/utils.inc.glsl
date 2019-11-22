//////////////////////////////////////////////////////
// Misc utility functions

const float pi = 3.1415926535897932384626433832795;

vec3 color2normal(in vec4 color) {
    return normalize(vec3(color) * 2.0 - vec3(1.0));
}

vec4 normal2color(in vec3 normal, in float alpha = 1.0) {
    return vec4(normal * 0.5 + vec3(0.5), alpha);
}

bool isOrthographic(mat4 projectionMatrix) {
	return abs(projectionMatrix[3][3]) > 0.01;
}
