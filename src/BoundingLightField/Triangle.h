//
// MIT licensed -- Copyright (c) 2017, Élie Michel
//
// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

using glm::vec3;

#include <iostream>

class Triangle {
public:
    vec3 a, b, c;

public:
    inline void boundingSphere(vec3 & center, float & radius) const {
        vec3 d12 = a - b;
        vec3 d23 = b - c;
        vec3 d31 = c - a;
        float sl12 = length2(d12);
        float sl23 = length2(d23);
        float sl31 = length2(d31);
        float s = 2 * length2(cross(d12, d23));
        float alpha = -sl23 * dot(d12, d31) / s;
        float beta = -sl31 * dot(d12, d23) / s;
        float gamma = -sl12 * dot(d31, d23) / s;

        radius = sqrt(sl12 * sl23 * sl31) / (2 * cross(d12, d23).length());
        center = alpha * a + beta * b + gamma * c;
    }

    inline float projectedArea(const vec3 & direction) const {
        vec3 u = b - a;
        vec3 v = c - a;
        // project onto plane
        u -= dot(u, direction) * direction;
        v -= dot(v, direction) * direction;
        // area
        return 0.5f * length(cross(u, v));
    }
};


class RichTriangle : public Triangle {
public:
    // normals
    vec3 na, nb, nc;
    int materialId;
};


class RichTexturedTriangle : public RichTriangle {
public:
	// normals
	glm::vec2 uva, uvb, uvc;
};


inline std::ostream & operator<<(std::ostream & os, const Triangle & tri) {
    os << "Triangle("
       << to_string(tri.a) << ", "
       << to_string(tri.b) << ", "
       << to_string(tri.c) << ")";

    return os;
}

