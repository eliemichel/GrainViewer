// requires random.inc.glsl

mat3 randomGrainOrientation(int id) {
    vec3 vx = normalize(randVec(vec3(id, id, id)));
    vec3 vy = normalize(cross(vec3(0,0,sign(vx) * 1 + .001), vx));
    vec3 vz = normalize(cross(vx, vy));
    float t = 0.0;
    return mat3(vx, vy * cos(t) - vz * sin(t), vy * sin(t) + vz * cos(t));
}

mat4 randomGrainMatrix(int id, vec3 position_ws = vec3(0.0, 0.0, 0.0)) {
    mat3 rot = randomGrainOrientation(id);
    vec3 position_gs = position_ws;
    return mat4(
        vec4(rot[0], 0.0),
        vec4(rot[1], 0.0),
        vec4(rot[2], 0.0),
        vec4(position_gs, 1.0)
    );
}
