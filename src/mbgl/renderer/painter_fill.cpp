#include <mbgl/renderer/painter.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/renderer/fill_bucket.hpp>
#include <mbgl/renderer/render_tile.hpp>
#include <mbgl/style/layers/fill_layer.hpp>
#include <mbgl/style/layers/fill_layer_impl.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/shader/shaders.hpp>
#include <mbgl/shader/fill_uniforms.hpp>
#include <mbgl/util/convert.hpp>

namespace mbgl {

using namespace style;

void Painter::renderFill(PaintParameters& parameters,
                         FillBucket& bucket,
                         const FillLayer& layer,
                         const RenderTile& tile) {
    const FillPaintProperties& properties = layer.impl->paint;

    FillUniforms baseUniforms {
        tile.translatedMatrix(properties.fillTranslate,
                              properties.fillTranslateAnchor,
                              state),
        properties.fillOpacity,
        frame.framebufferSize
    };

    if (!properties.fillPattern.value.from.empty()) {
        if (pass != RenderPass::Translucent) {
            return;
        }

        optional<SpriteAtlasPosition> imagePosA = spriteAtlas->getPosition(
            properties.fillPattern.value.from, SpritePatternMode::Repeating);
        optional<SpriteAtlasPosition> imagePosB = spriteAtlas->getPosition(
            properties.fillPattern.value.to, SpritePatternMode::Repeating);

        if (!imagePosA || !imagePosB) {
            return;
        }

        FillPatternUniforms uniforms {
            *imagePosA,
            *imagePosB,
            properties.fillPattern.value,
            tile.id,
            state
        };

        spriteAtlas->bind(true, context, 0);

        context.draw({
            gl::Depth { gl::Depth::LessEqual, true, depthRangeForSublayer(0) },
            stencilForClipping(tile.clip),
            colorForRenderPass(),
            parameters.shaders.fillPattern,
            std::make_tuple(baseUniforms, uniforms),
            gl::Segmented<gl::Triangles>(
                *bucket.vertexBuffer,
                *bucket.triangleIndexBuffer,
                bucket.triangleSegments
            )
        });

        if (!properties.fillAntialias) {
            return;
        }

        context.draw({
            gl::Depth { gl::Depth::LessEqual, true, depthRangeForSublayer(2) },
            stencilForClipping(tile.clip),
            colorForRenderPass(),
            parameters.shaders.fillOutlinePattern,
            std::make_tuple(baseUniforms, uniforms),
            gl::Segmented<gl::Lines>(
                *bucket.vertexBuffer,
                *bucket.lineIndexBuffer,
                bucket.lineSegments,
                2.0f
            )
        });
    } else {
        if (properties.fillAntialias && !properties.fillOutlineColor.isUndefined() && pass == RenderPass::Translucent) {
            context.draw({
                gl::Depth { gl::Depth::LessEqual, true, depthRangeForSublayer(2) },
                stencilForClipping(tile.clip),
                colorForRenderPass(),
                parameters.shaders.fillOutline,
                std::make_tuple(baseUniforms, FillColorUniforms {
                    properties.fillOutlineColor
                }),
                gl::Segmented<gl::Lines>(
                    *bucket.vertexBuffer,
                    *bucket.lineIndexBuffer,
                    bucket.lineSegments,
                    2.0f
                )
            });
        }

        // Only draw the fill when it's either opaque and we're drawing opaque
        // fragments or when it's translucent and we're drawing translucent
        // fragments
        if ((properties.fillColor.value.a >= 1.0f && properties.fillOpacity.value >= 1.0f) == (pass == RenderPass::Opaque)) {
            context.draw({
                gl::Depth { gl::Depth::LessEqual, true, depthRangeForSublayer(1) },
                stencilForClipping(tile.clip),
                colorForRenderPass(),
                parameters.shaders.fill,
                std::make_tuple(baseUniforms, FillColorUniforms {
                    properties.fillColor
                }),
                gl::Segmented<gl::Triangles>(
                    *bucket.vertexBuffer,
                    *bucket.triangleIndexBuffer,
                    bucket.triangleSegments
                )
            });
        }

        if (properties.fillAntialias && properties.fillOutlineColor.isUndefined() && pass == RenderPass::Translucent) {
            context.draw({
                gl::Depth { gl::Depth::LessEqual, true, depthRangeForSublayer(2) },
                stencilForClipping(tile.clip),
                colorForRenderPass(),
                parameters.shaders.fillOutline,
                std::make_tuple(baseUniforms, FillColorUniforms {
                    properties.fillColor
                }),
                gl::Segmented<gl::Lines>(
                    *bucket.vertexBuffer,
                    *bucket.lineIndexBuffer,
                    bucket.lineSegments,
                    2.0f
                )
            });
        }
    }
}

} // namespace mbgl
