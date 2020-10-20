#pragma once

#include <refl.hpp>

namespace ReflectionAttributes {

struct HideInDialog : refl::attr::usage::member {
    constexpr HideInDialog() { }
};

struct Range : refl::attr::usage::member {
    const float minimum;
    const float maximum;
    constexpr Range(const float minimum, const float maximum)
        : minimum(minimum), maximum(maximum)
    {}
};

} // ReflectionAttributes
