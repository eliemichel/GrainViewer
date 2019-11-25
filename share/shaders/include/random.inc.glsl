//////////////////////////////////////////////////////
// Random generation functions

float randv2(vec2 co){
    return fract(sin(dot(co ,vec2(12.9898,78.233))) * 43758.5453);
}
float rand(vec3 co){
    return randv2(vec2(randv2(co.xy/100.0f), randv2(co.yz/100.0f)));
}
float rand2(vec3 co){
    float a = rand(co);
    return rand(vec3(a,a+1,a+2));
}
float rand3(vec3 co){
    float a = rand(co);
    float b = rand(co);
    return rand(vec3(a,b,a+b));
}
vec3 randVec(vec3 co) {
    return vec3(rand(co), rand2(co), rand3(co)) * 2.f - vec3(1.f);
}

float length2(vec3 v) {
    return dot(v,v);
}
