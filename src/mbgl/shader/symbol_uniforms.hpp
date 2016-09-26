#pragma once

#include <mbgl/util/mat4.hpp>

namespace mbgl {

class SymbolUniforms {
public:
    mat4 matrix;
    float opacity;
    std::array<float, 2> extrudeScale;
    std::array<float, 2> texsize;
    int32_t rotateWithMap;
    float zoom;

    int32_t texture = 0;
    int32_t fadeTexture = 1;
};

class SymbolSDFUniforms {
public:
    Color color;
    float buffer;
    float gamma;
    float pitch;
    float bearing;
    float aspectRatio;
    int32_t pitchWithMap;
};

} // namespace mbgl
