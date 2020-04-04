// just a regular post effect

layout (location = 0) in vec4 position;

out vec2 vertUv;

void main() {
	gl_Position = vec4(position.xyz, 1.0);
	vertUv = position.xy * .5 + .5;
}
