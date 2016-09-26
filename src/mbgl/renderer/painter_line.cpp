#include <mbgl/renderer/painter.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/renderer/line_bucket.hpp>
#include <mbgl/renderer/render_tile.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layers/line_layer_impl.hpp>
#include <mbgl/shader/shaders.hpp>
#include <mbgl/shader/line_uniforms.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/geometry/line_atlas.hpp>

namespace mbgl {

using namespace style;

void Painter::renderLine(PaintParameters& parameters,
                         LineBucket& bucket,
                         const LineLayer& layer,
                         const RenderTile& tile) {
    if (pass == RenderPass::Opaque) {
        return;
    }

    const auto& properties = layer.impl->paint;

    auto draw = [&] (auto& shader, const auto& uniforms) {
        // the distance over which the line edge fades out.
        // Retina devices need a smaller distance to avoid aliasing.
        float antialiasing = 1.0 / frame.pixelRatio;

        mat2 antialiasingMatrix;
        matrix::identity(antialiasingMatrix);
        matrix::scale(antialiasingMatrix, antialiasingMatrix, 1.0, std::cos(state.getPitch()));
        matrix::rotate(antialiasingMatrix, antialiasingMatrix, state.getAngle());

        // calculate how much longer the real world distance is at the top of the screen
        // than at the middle of the screen.
        float topedgelength = std::sqrt(std::pow(state.getHeight(), 2.0f) / 4.0f  * (1.0f + std::pow(state.getAltitude(), 2.0f)));
        float x = state.getHeight() / 2.0f * std::tan(state.getPitch());

        LineUniforms baseUniforms {
            tile.translatedMatrix(properties.lineTranslate,
                                  properties.lineTranslateAnchor,
                                  state),
            properties.lineOpacity,
            properties.lineWidth / 2,
            properties.lineGapWidth / 2,
            properties.lineBlur + antialiasing,
            -properties.lineOffset,
            antialiasing / 2,
            antialiasingMatrix,
            1.0f / tile.id.pixelsToTileUnits(1.0, state.getZoom()),
            (topedgelength + x) / topedgelength - 1.0f
        };

        context.draw({
            gl::Depth { gl::Depth::LessEqual, false, depthRangeForSublayer(0) },
            stencilForClipping(tile.clip),
            colorForRenderPass(),
            shader,
            std::make_tuple(baseUniforms, uniforms),
            gl::Segmented<gl::Triangles>(
                *bucket.vertexBuffer,
                *bucket.indexBuffer,
                bucket.segments
            )
        });
    };

    if (!properties.lineDasharray.value.from.empty()) {
        const LinePatternCap cap =
            bucket.layout.lineCap == LineCapType::Round ? LinePatternCap::Round : LinePatternCap::Square;
        LinePatternPos posA = lineAtlas->getDashPosition(properties.lineDasharray.value.from, cap);
        LinePatternPos posB = lineAtlas->getDashPosition(properties.lineDasharray.value.to, cap);

        const float widthA = posA.width * properties.lineDasharray.value.fromScale * layer.impl->dashLineWidth;
        const float widthB = posB.width * properties.lineDasharray.value.toScale * layer.impl->dashLineWidth;

        float scaleXA = 1.0 / tile.id.pixelsToTileUnits(widthA, state.getIntegerZoom());
        float scaleYA = -posA.height / 2.0;
        float scaleXB = 1.0 / tile.id.pixelsToTileUnits(widthB, state.getIntegerZoom());
        float scaleYB = -posB.height / 2.0;

        lineAtlas->bind(context, 0);

        draw(parameters.shaders.lineSDF, LineSDFUniforms {
            properties.lineColor,
            {{ scaleXA, scaleYA }}, posA.y,
            {{ scaleXB, scaleYB }}, posB.y,
            properties.lineDasharray.value.t,
            lineAtlas->width / (std::min(widthA, widthB) * 256.0f * frame.pixelRatio) / 2.0f
        });
    } else if (!properties.linePattern.value.from.empty()) {
        optional<SpriteAtlasPosition> imagePosA = spriteAtlas->getPosition(
            properties.linePattern.value.from, SpritePatternMode::Repeating);
        optional<SpriteAtlasPosition> imagePosB = spriteAtlas->getPosition(
            properties.linePattern.value.to, SpritePatternMode::Repeating);

        if (!imagePosA || !imagePosB)
            return;

        spriteAtlas->bind(true, context, 0);

        draw(parameters.shaders.linePattern, LinePatternUniforms {
            {{
                tile.id.pixelsToTileUnits((*imagePosA).size[0] * properties.linePattern.value.fromScale, state.getIntegerZoom()),
                (*imagePosA).size[1]
            }},
            (*imagePosA).tl,
            (*imagePosA).br,
            {{
                tile.id.pixelsToTileUnits((*imagePosB).size[0] * properties.linePattern.value.toScale, state.getIntegerZoom()),
                (*imagePosB).size[1]
            }},
            (*imagePosB).tl,
            (*imagePosB).br,
            properties.linePattern.value.t
        });
    } else {
        draw(parameters.shaders.line, LineColorUniforms {
            properties.lineColor
        });
    }
}

} // namespace mbgl
