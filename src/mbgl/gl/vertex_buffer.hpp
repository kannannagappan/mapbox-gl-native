#pragma once

#include <mbgl/gl/object.hpp>

namespace mbgl {
namespace gl {

template <class Vertex>
class VertexBuffer {
public:
    using VertexType = Vertex;
    static constexpr std::size_t vertexSize = sizeof(Vertex);
    static constexpr std::size_t vertexOffset = 0;
    std::size_t vertexCount;
    UniqueBuffer buffer;
};

} // namespace gl
} // namespace mbgl
