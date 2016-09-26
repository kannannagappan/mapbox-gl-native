#include <mbgl/shader/fill_uniforms.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/style/property_evaluator.hpp>
#include <mbgl/tile/tile_id.hpp>
#include <mbgl/map/transform_state.hpp>

namespace mbgl {

using namespace style;

FillPatternUniforms::FillPatternUniforms(const SpriteAtlasPosition& a,
                                         const SpriteAtlasPosition& b,
                                         const Faded<std::string>& fading,
                                         const UnwrappedTileID& tileID,
                                         const TransformState& state)
{
    patternTLA = a.tl;
    patternBRA = a.br;
    patternTLB = b.tl;
    patternBRB = b.br;
    patternSizeA = a.size;
    patternSizeB = b.size;

    scaleA = fading.fromScale;
    scaleB = fading.toScale;
    mix = fading.t;

    tileUnitsToPixels = 1.0f / tileID.pixelsToTileUnits(1.0f, state.getIntegerZoom());

    int32_t tileSizeAtNearestZoom = util::tileSize * state.zoomScale(state.getIntegerZoom() - tileID.canonical.z);
    int32_t pixelX = tileSizeAtNearestZoom * (tileID.canonical.x + tileID.wrap * state.zoomScale(tileID.canonical.z));
    int32_t pixelY = tileSizeAtNearestZoom * tileID.canonical.y;

    pixelCoordUpper = {{ float(pixelX >> 16), float(pixelY >> 16) }};
    pixelCoordLower = {{ float(pixelX & 0xFFFF), float(pixelY & 0xFFFF) }};
}

} // namespace mbgl
