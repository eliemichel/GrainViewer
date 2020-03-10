#version 450 core
#include "sys:defines"

in vec3 position_ws;
in vec3 normal_ws;
in vec3 normal_cs;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;

layout (location = 0) out vec4 out_color;

uniform vec4 baseColor = vec4(1.0, 0.5, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 1.0;
uniform float roughness = 0.7;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 1.0;

void main() {
    out_color = baseColor;
}
