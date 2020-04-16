
in vec2 vertUv[];
in flat int vertLayer[];
out vec2 uv;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// this is just a passthrough geo shader
void main() {
	gl_Layer = vertLayer[0];
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Position = gl_in[i].gl_Position;
		uv = vertUv[i];
		EmitVertex();
	}
	EndPrimitive();
}
