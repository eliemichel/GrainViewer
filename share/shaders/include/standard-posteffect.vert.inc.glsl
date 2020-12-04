// just a regular post effect

layout (location = 0) in vec4 position;

out vec2 vertUv;
out flat int vertLayer;

void main() {
	gl_Position = vec4(position.xyz, 1.0);
	vertUv = position.xy * .5 + .5;
	vertLayer = gl_InstanceID;
}
