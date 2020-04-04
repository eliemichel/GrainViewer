// generic passthrough geometry shader
#extension GL_EXT_geometry_shader4 : enable // to use gl_PositionIn[]

in vec2 vertUv[];
out vec2 uv;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// this is just a passthrough geo shader
void main() {
	for (int i = 0; i < gl_VerticesIn; i++) {
		gl_Position = gl_PositionIn[i];
		uv = vertUv[i];
		EmitVertex();
	}
	EndPrimitive();
}
