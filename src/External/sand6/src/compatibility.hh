/**
 * Copyright (c) 2020 - Elie Michel
 * Snippets added for compatibility when porting sand6 to MSVC
 */
#pragma once

#ifdef _WIN32

#include <limits>

// Compile time sqrt
// https://stackoverflow.com/questions/8622256/in-c11-is-sqrt-defined-as-constexpr
namespace Detail
{
    double constexpr sqrtNewtonRaphson(double x, double curr, double prev)
    {
        return curr == prev
            ? curr
            : sqrtNewtonRaphson(x, 0.5 * (curr + x / curr), curr);
    }
}

namespace compat {

/**
 * Constexpr version of the square root
 * Return value:
 *   - For a finite and non-negative value of "x", returns an approximation for the square root of "x"
 *   - Otherwise, returns NaN
 */
double constexpr sqrt(double x)
{
    return x >= 0 && x < std::numeric_limits<double>::infinity()
        ? Detail::sqrtNewtonRaphson(x, x, 0)
        : std::numeric_limits<double>::quiet_NaN();
}

} // std

static constexpr double M_SQRT2 = compat::sqrt(2.);
static constexpr double M_PI = 3.14159265358979323846;
static constexpr double M_PI_2 = M_PI / 2;

#else // _WIN32

#define compat std

#endif // _WIN32
