#pragma once

#include <mbgl/util/mat4.hpp>
#include <mbgl/util/color.hpp>

#include <array>
#include <string>

namespace mbgl {

class FillUniforms {
public:
    mat4 matrix;
    float opacity;
    std::array<uint16_t, 2> framebufferSize;
};

class FillColorUniforms {
public:
    Color color;
};

class SpriteAtlasPosition;
class UnwrappedTileID;
class TransformState;

namespace style {
template <class> class Faded;
} // namespace style

class FillPatternUniforms {
public:
    FillPatternUniforms(const SpriteAtlasPosition&,
                        const SpriteAtlasPosition&,
                        const style::Faded<std::string>&,
                        const UnwrappedTileID&,
                        const TransformState&);

    // TODO: dedup with LinePattern
    std::array<float, 2> patternSizeA;
    std::array<float, 2> patternTLA;
    std::array<float, 2> patternBRA;
    float scaleA;
    std::array<float, 2> patternSizeB;
    std::array<float, 2> patternTLB;
    std::array<float, 2> patternBRB;
    float scaleB;
    float mix;
    float tileUnitsToPixels;
    std::array<float, 2> pixelCoordUpper;
    std::array<float, 2> pixelCoordLower;
    float textureUnit = 0;
};

} // namespace mbgl
