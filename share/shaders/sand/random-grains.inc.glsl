// requires random.inc.glsl
#pragma variant NO_GRAIN_ROTATION

mat3 randomGrainOrientation(int id) {
    vec3 vx = normalize(randVec(vec3(id, id, id)));
    vec3 vy = normalize(cross(vec3(0,0,sign(vx) * 1 + .001), vx));
    vec3 vz = normalize(cross(vx, vy));
    float t = 0.0;
    return mat3(vx, vy * cos(t) - vz * sin(t), vy * sin(t) + vz * cos(t));
}

mat4 randomGrainMatrix(int id, vec3 position_ws) {
#ifdef NO_GRAIN_ROTATION
    mat3 rot = mat3(1.0);
#else // NO_GRAIN_ROTATION
    mat3 rot = randomGrainOrientation(id);
#endif // NO_GRAIN_ROTATION
    return mat4(
        vec4(rot[0], 0.0),
        vec4(rot[1], 0.0),
        vec4(rot[2], 0.0),
        vec4(position_ws, 1.0)
    );
}

// Used to query colorramp image
float randomGrainColorFactor(int id) {
    return randv2(vec2(id, id));
}
