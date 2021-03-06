//////////////////////////////////////////////////////
// Ray-tracing routines

// requires that you include utils.inc.glsl for isOrthographic() and uniforms/camera.inc.glsl for resolution

struct Ray {
    vec3 origin;
    vec3 direction;
};

/**
 * Apply an affine transformation matrix to a ray
 */
Ray TransformRay(Ray ray, mat4 transform) {
    Ray transformed_ray;
    transformed_ray.origin = (transform * vec4(ray.origin, 1)).xyz;
    transformed_ray.direction = mat3(transform) * ray.direction;
    return transformed_ray;
}

/**
 * Get the ray corresponding to a pixel, in camera space
 */
Ray fragmentRay(in vec4 fragCoord, in mat4 projectionMatrix) {
    vec2 uv = fragCoord.xy / resolution.xy * 2.f - vec2(1.f);
    
    if (isOrthographic(projectionMatrix)) {
        float a = 1.0 / projectionMatrix[0][0];
        float b = a * projectionMatrix[3][0];
        float c = 1.0 / projectionMatrix[1][1];
        float d = c * projectionMatrix[3][1];
        return Ray(
            vec3(a * uv.x + b, c * uv.y + d, 0.0),
            vec3(0.0, 0.0, -1.f)
        );
    } else {
        float fx = projectionMatrix[0][0];
        float fy = projectionMatrix[1][1];
        return Ray(
            vec3(0.0),
            vec3(uv.x / fx, uv.y / fy, -1.f)
        );
    }
}


bool intersectRaySphere(out vec3 hitPosition, in Ray ray, in vec3 center, in float radius) {
    vec3 o = center - ray.origin;
    float d2 = dot(ray.direction, ray.direction);
    float r2 = radius * radius;
    float l2 = dot(o, o);
    float dp = dot(ray.direction, o);
    float delta = dp * dp / d2 - l2 + r2;

    if (delta >= 0) {
        float lambda = dp / d2 - sqrt(delta / d2);
        if (lambda < 0) lambda += 2. * sqrt(delta / d2);
        if (lambda >= 0) {
            hitPosition = ray.origin + lambda * ray.direction;
            return true;
        }
    }

    return false;
}


vec3 intersectRayPlane(in Ray ray, in vec3 pointOnPlane, in vec3 planeNormal) {
    vec3 o = pointOnPlane - ray.origin;
    float lambda = dot(o, planeNormal) / dot(ray.direction, planeNormal);
    return ray.origin + lambda * ray.direction;
}

