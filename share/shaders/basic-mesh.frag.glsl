#version 450 core
#include "sys:defines"

in vec3 position_ws;
in vec3 normal_ws;
in vec3 normal_cs;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;

layout (location = 0) out vec4 color;

uniform vec4 baseColor = vec4(1.0, 0.5, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 1.0;
uniform float roughness = 0.5;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;

uniform vec3 light0_position = vec3(2.0, 2.0, 3.0);
uniform vec3 light1_position = vec3(1.0, -2.0, 2.0);
uniform vec3 light2_position = vec3(-1.0, 2.0, -1.0);

#include "include/utils.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
	mat3 TBN = mat3(
        normalize(cross(normal_ws, tangent_ws)),
        normalize(tangent_ws),
        normalize(normal_ws)
    );
    vec3 normal = normalize(normal_ws + TBN * (normal * 2.0 - 1.0) * normal_mapping);

    vec3 c = baseColor.rgb * vec3(0.1, 0.15, 0.2);
    vec3 toLight;

    toLight = -normalize(position_ws - light0_position);
	c += bsdfPbrMetallicRoughness(position_ws, toLight, normal_ws, baseColor.rgb, roughness, metallic);
	toLight = -normalize(position_ws - light1_position);
	c += bsdfPbrMetallicRoughness(position_ws, toLight, normal_ws, baseColor.rgb, roughness, metallic);
	toLight = -normalize(position_ws - light2_position);
	c += bsdfPbrMetallicRoughness(position_ws, toLight, normal_ws, baseColor.rgb, roughness, metallic);

	color = vec4(c, 1.0);
}
