#pragma once

#include <mbgl/util/mat4.hpp>
#include <mbgl/util/mat2.hpp>
#include <mbgl/util/color.hpp>

#include <array>

namespace mbgl {

class LineUniforms {
public:
    mat4 matrix;
    float opacity;
    float lineWidth;
    float gapWidth;
    float blur;
    float offset;
    float antialiasing;
    mat2 antialiasingMatrix;
    float ratio;
    float extra;
};

class LineColorUniforms {
public:
    Color color;
};

class LineSDFUniforms {
public:
    Color color;
    std::array<float, 2> patternScaleA;
    float texYA;
    std::array<float, 2> patternScaleB;
    float texYB;
    float mix;
    float sdfGamma;
    float textureUnit = 0;
};

class LinePatternUniforms {
public:
    std::array<float, 2> patternSizeA;
    std::array<float, 2> patternTLA;
    std::array<float, 2> patternBRA;
    std::array<float, 2> patternSizeB;
    std::array<float, 2> patternTLB;
    std::array<float, 2> patternBRB;
    float mix;
    float textureUnit = 0;
};

} // namespace mbgl
