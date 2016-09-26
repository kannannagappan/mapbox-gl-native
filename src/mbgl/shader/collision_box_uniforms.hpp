#pragma once

#include <mbgl/util/mat4.hpp>

namespace mbgl {

class CollisionBoxUniforms {
public:
    mat4 matrix;
    float scale;
    float zoom;
    float maxZoom;
};

} // namespace mbgl
