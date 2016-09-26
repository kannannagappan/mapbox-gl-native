#include <mbgl/renderer/painter.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/renderer/symbol_bucket.hpp>
#include <mbgl/renderer/render_tile.hpp>
#include <mbgl/style/layers/symbol_layer.hpp>
#include <mbgl/style/layers/symbol_layer_impl.hpp>
#include <mbgl/text/glyph_atlas.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/shader/shaders.hpp>
#include <mbgl/shader/symbol_uniforms.hpp>
#include <mbgl/shader/collision_box_uniforms.hpp>
#include <mbgl/util/math.hpp>

#include <cmath>

namespace mbgl {

using namespace style;

void Painter::renderSymbol(PaintParameters& parameters,
                           SymbolBucket& bucket,
                           const SymbolLayer& layer,
                           const RenderTile& tile) {
    if (pass == RenderPass::Opaque) {
        return;
    }

    const auto& paint = layer.impl->paint;
    const auto& layout = bucket.layout;

    gl::Depth depth {
        gl::Depth::LessEqual,
        false,
        depthRangeForSublayer(0)
    };

    // TODO remove the `true ||` when #1673 is implemented
    const bool drawAcrossEdges = (frame.mapMode == MapMode::Continuous) && (true || !(layout.textAllowOverlap || layout.iconAllowOverlap ||
          layout.textIgnorePlacement || layout.iconIgnorePlacement));

    // Disable the stencil test so that labels aren't clipped to tile boundaries.
    //
    // Layers with features that may be drawn overlapping aren't clipped. These
    // layers are sorted in the y direction, and to draw the correct ordering near
    // tile edges the icons are included in both tiles and clipped when drawing.

    gl::Stencil stencil = drawAcrossEdges
        ? gl::Stencil::disabled()
        : stencilForClipping(tile.clip);

    auto drawSDFs = [&] (const auto& buffers,
                         auto& shader,
                         float sdfFontSize,
                         std::array<float, 2> texsize,

                         // Layout
                         AlignmentType rotationAlignment,
                         AlignmentType pitchAlignment,
                         float layoutSize,

                         // Paint
                         float opacity,
                         Color color,
                         Color haloColor,
                         float haloWidth,
                         float haloBlur,
                         std::array<float, 2> translate,
                         TranslateAnchorType translateAnchor,
                         float paintSize)
    {
        mat4 matrix = tile.translatedMatrix(translate, translateAnchor, state);

        // If layerStyle.size > bucket.info.fontSize then labels may collide
        float fontSize = paintSize;
        float fontScale = fontSize / sdfFontSize;

        bool rotateWithMap = rotationAlignment == AlignmentType::Map;
        bool pitchWithMap = pitchAlignment == AlignmentType::Map;

        std::array<float, 2> extrudeScale;
        float gammaScale;

        if (pitchWithMap) {
            gammaScale = 1.0 / std::cos(state.getPitch());
            extrudeScale.fill(tile.id.pixelsToTileUnits(1, state.getZoom()) * fontScale);
        } else {
            gammaScale = 1.0;
            extrudeScale = {{
                pixelsToGLUnits[0] * fontScale * state.getAltitude(),
                pixelsToGLUnits[1] * fontScale * state.getAltitude()
            }};
        }

        // adjust min/max zooms for variable font sies
        float zoomAdjust = std::log(fontSize / layoutSize) / std::log(2);

        // The default gamma value has to be adjust for the current pixelratio so that we're not
        // drawing blurry font on retina screens.
        const float gamma = 0.105 * sdfFontSize / fontSize / frame.pixelRatio;

        SymbolUniforms baseUniforms {
            matrix,
            opacity,
            extrudeScale,
            texsize,
            rotateWithMap,
            (state.getZoom() - zoomAdjust) * 10.0f
        };

        frameHistory.bind(context, 1);

        // We're drawing in the translucent pass which is bottom-to-top, so we need
        // to draw the halo first.
        if (haloColor.a > 0.0f && haloWidth > 0.0f) {
            const float sdfPx = 8.0f;
            const float blurOffset = 1.19f;
            const float haloOffset = 6.0f;

            context.draw({
                depth,
                stencil,
                colorForRenderPass(),
                shader,
                std::make_tuple(baseUniforms, SymbolSDFUniforms {
                    haloColor,
                    (haloOffset - haloWidth / fontScale) / sdfPx,
                    (haloBlur * blurOffset / fontScale / sdfPx + gamma) * gammaScale,
                    state.getPitch(),
                    -1.0f * state.getAngle(),
                    (state.getWidth() * 1.0f) / (state.getHeight() * 1.0f),
                    pitchWithMap
                }),
                gl::Segmented<gl::Triangles>(
                    *buffers.vertexBuffer,
                    *buffers.indexBuffer,
                    buffers.segments
                )
            });
        }

        // Then, we draw the text/icon over the halo
        if (color.a > 0.0f) {
            context.draw({
                depth,
                stencil,
                colorForRenderPass(),
                shader,
                std::make_tuple(baseUniforms, SymbolSDFUniforms {
                    color,
                    (256.0f - 64.0f) / 256.0f,
                    gamma * gammaScale,
                    state.getPitch(),
                    -1.0f * state.getAngle(),
                    (state.getWidth() * 1.0f) / (state.getHeight() * 1.0f),
                    pitchWithMap
                }),
                gl::Segmented<gl::Triangles>(
                    *buffers.vertexBuffer,
                    *buffers.indexBuffer,
                    buffers.segments
                )
            });
        }
    };

    if (bucket.hasIconData()) {
        if (layout.iconRotationAlignment == AlignmentType::Map) {
            context.depthFunc = gl::DepthTestFunction::LessEqual;
            context.depthTest = true;
        } else {
            context.depthTest = false;
        }

        bool sdf = bucket.sdfIcons;

        const float angleOffset =
            layout.iconRotationAlignment == AlignmentType::Map
                ? state.getAngle()
                : 0;

        const float fontSize = layer.impl->iconSize;
        const float fontScale = fontSize / 1.0f;

        SpriteAtlas* activeSpriteAtlas = layer.impl->spriteAtlas;
        const bool iconScaled = fontScale != 1 || frame.pixelRatio != activeSpriteAtlas->getPixelRatio() || bucket.iconsNeedLinear;
        const bool iconTransformed = layout.iconRotationAlignment == AlignmentType::Map || angleOffset != 0 || state.getPitch() != 0;
        activeSpriteAtlas->bind(sdf || state.isChanging() || iconScaled || iconTransformed, context, 0);

        if (sdf) {
            drawSDFs(bucket.icon,
                     parameters.shaders.symbolIconSDF,
                     1.0f,
                     {{ float(activeSpriteAtlas->getWidth()) / 4.0f, float(activeSpriteAtlas->getHeight()) / 4.0f }},
                     layout.iconRotationAlignment,
                     // icon-pitch-alignment is not yet implemented
                     // and we simply inherit the rotation alignment
                     layout.iconRotationAlignment,
                     layout.iconSize,
                     paint.iconOpacity,
                     paint.iconColor,
                     paint.iconHaloColor,
                     paint.iconHaloWidth,
                     paint.iconHaloBlur,
                     paint.iconTranslate,
                     paint.iconTranslateAnchor,
                     layer.impl->iconSize);
        } else {
            mat4 matrix = tile.translatedMatrix(paint.iconTranslate,
                                                   paint.iconTranslateAnchor,
                                                   state);

            std::array<float, 2> extrudeScale;

            const bool alignedWithMap = layout.iconRotationAlignment == AlignmentType::Map;
            if (alignedWithMap) {
                extrudeScale.fill(tile.id.pixelsToTileUnits(1, state.getZoom()) * fontScale);
            } else {
                extrudeScale = {{
                    pixelsToGLUnits[0] * fontScale * state.getAltitude(),
                    pixelsToGLUnits[1] * fontScale * state.getAltitude()
                }};
            }

            // adjust min/max zooms for variable font sies
            float zoomAdjust = std::log(fontSize / layout.iconSize) / std::log(2);

            frameHistory.bind(context, 1);

            context.draw({
                depth,
                stencil,
                colorForRenderPass(),
                parameters.shaders.symbolIcon,
                SymbolUniforms {
                    matrix,
                    paint.iconOpacity,
                    extrudeScale,
                    {{ float(activeSpriteAtlas->getWidth()) / 4.0f, float(activeSpriteAtlas->getHeight()) / 4.0f }},
                    alignedWithMap,
                    (state.getZoom() - zoomAdjust) * 10.0f
                },
                gl::Segmented<gl::Triangles>(
                    *bucket.icon.vertexBuffer,
                    *bucket.icon.indexBuffer,
                    bucket.icon.segments
                )
            });
        }
    }

    if (bucket.hasTextData()) {
        if (layout.textPitchAlignment == AlignmentType::Map) {
            context.depthFunc = gl::DepthTestFunction::LessEqual;
            context.depthTest = true;
        } else {
            context.depthTest = false;
        }

        glyphAtlas->bind(context, 0);

        drawSDFs(bucket.text,
                 parameters.shaders.symbolGlyph,
                 24.0f,
                 {{ float(glyphAtlas->width) / 4, float(glyphAtlas->height) / 4 }},
                 layout.textRotationAlignment,
                 layout.textPitchAlignment,
                 layout.textSize,
                 paint.textOpacity,
                 paint.textColor,
                 paint.textHaloColor,
                 paint.textHaloWidth,
                 paint.textHaloBlur,
                 paint.textTranslate,
                 paint.textTranslateAnchor,
                 layer.impl->textSize);
    }

    context.draw({
        depth,
        stencilForClipping(tile.clip),
        colorForRenderPass(),
        shaders->collisionBox,
        CollisionBoxUniforms {
            tile.matrix,
            std::pow(2, state.getZoom() - tile.id.canonical.z),
            state.getZoom() * 10,
            (tile.id.canonical.z + 1) * 10
        },
        gl::Segmented<gl::Lines>(
            *bucket.collisionBox.vertexBuffer,
            *bucket.collisionBox.indexBuffer,
            bucket.collisionBox.segments,
            1.0f
        )
    });
}

} // namespace mbgl
