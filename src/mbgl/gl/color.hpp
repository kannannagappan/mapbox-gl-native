#pragma once

#include <mbgl/util/variant.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {
namespace gl {

class Color {
public:
    enum BlendFactor {
        Zero                  = 0x0000,
        One                   = 0x0001,
        SrcColor              = 0x0300,
        OneMinusSrcColor      = 0x0301,
        SrcAlpha              = 0x0302,
        OneMinusSrcAlpha      = 0x0303,
        DstAlpha              = 0x0304,
        OneMinusDstAlpha      = 0x0305,
        DstColor              = 0x0306,
        OneMinusDstColor      = 0x0307,
        SrcAlphaSaturate      = 0x0308,
        ConstantColor         = 0x8001,
        OneMinusConstantColor = 0x8002,
        ConstantAlpha         = 0x8003,
        OneMinusConstantAlpha = 0x8004
    };

    template <uint32_t>
    struct ConstantBlend {
    };

    template <uint32_t>
    struct LinearBlend {
        BlendFactor srcFactor;
        BlendFactor dstFactor;
    };

    struct Replace {};
    using Min              = ConstantBlend<0x8007>;
    using Max              = ConstantBlend<0x8008>;
    using Add              = LinearBlend<0x8006>;
    using Subtract         = LinearBlend<0x800A>;
    using ReverseSubtract  = LinearBlend<0x800B>;

    using BlendFunction = variant<
        Replace,
        Min,
        Max,
        Add,
        Subtract,
        ReverseSubtract>;

    BlendFunction blendFunction;
    mbgl::Color blendColor;

    struct Mask {
        bool r;
        bool g;
        bool b;
        bool a;
    };

    Mask mask;

    static Color disabled() {
       return Color { Replace(), {}, { false, false, false, false } };
    }

    static Color unblended() {
       return Color { Replace(), {}, { true, true, true, true } };
    }

    static Color alphaBlended() {
        return Color { Add { One, OneMinusSrcAlpha }, {}, { true, true, true, true } };
    }
};

} // namespace gl
} // namespace mbgl
