#version 450 core

in vec4 grainAlbedo;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(grainAlbedo.rgb, 1.0);
}

