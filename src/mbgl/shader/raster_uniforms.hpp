#pragma once

#include <mbgl/util/mat4.hpp>

namespace mbgl {

class RasterUniforms {
public:
    mat4 matrix;
//    int32_t image0;
    float opacity0;
//    int32_t image1;
    float opacity1;
    float brightnessMin;
    float brightnessMax;
    float saturationFactor;
    float contrastFactor;
    std::array<float, 3> spinWeights;
    std::array<float, 2> tlParent = {{ 0.0f, 0.0f }};
    float scaleParent = 1.0f;
    float bufferScale = 1.0f;
};

} // namespace mbgl
