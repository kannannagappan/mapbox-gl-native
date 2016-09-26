#pragma once

#include <mbgl/util/mat4.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {

class CircleUniforms {
public:
    mat4 matrix;
    Color color;
    float opacity;
    float radius;
    float blur;
    float devicePixelRatio;
    std::array<float, 2> extrudeScale;
    bool scaleWithMap;
};

} // namespace mbgl
