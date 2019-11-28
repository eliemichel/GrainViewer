#version 450 core

in vec3 direction;

layout (location = 0) out vec4 color1;
layout (location = 1) out uvec4 color2;
layout (location = 2) out uvec4 color3;

uniform samplerCube cubemap;
uniform samplerCubeArray filteredCubemaps;

const uint nbSamples = 1024;

#include "include/gbuffer.inc.glsl"

void main() {
	//vec4 color = texture(cubemap, direction);

	GFragment fragment;
	//fragment.baseColor = vec3(color);
    fragment.ws_coord = vec3(direction * 1000.0);
    fragment.material_id = worldMaterial;
    fragment.normal = direction;

    packGFragment(fragment, color1, color2, color3);
}
