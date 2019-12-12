#version 450 core
#include "sys:defines"

#pragma variant ALL_BLACK

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
	fragment.baseColor = vec3(1.0, 0.0, 0.0);
    fragment.ws_coord = vec3(direction * 1000.0);
    fragment.material_id = worldMaterial;
    fragment.normal = direction;

#ifdef ALL_BLACK
    color1 = vec4(0.001);
    color2 = uvec4(0.0);
    color3 = uvec4(0.0);
#else // ALL_BLACK
    packGFragment(fragment, color1, color2, color3);
#endif // ALL_BLACK
}
