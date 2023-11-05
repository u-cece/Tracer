#pragma once

#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include <glm/glm.hpp>

#include "object.h"

namespace tracer
{

struct Vertex
{
    glm::vec3 pos;
    
};

class Mesh : public SimpleMaterialObject
{
public:
    template <std::ranges::input_range VertexRange,
              std::ranges::input_range IndexRange>
        requires
            std::is_same_v<
                std::ranges::range_value_t<VertexRange>,
                Vertex
            > &&
            std::is_same_v<
                std::ranges::range_value_t<IndexRange>,
                glm::u32vec3
            >
    static std::unique_ptr<Mesh> Create(VertexRange&& vertices, IndexRange&& indices)
    {
        VerticesType v = vertices | std::ranges::to<std::vector>();
        IndicesType i = indices | std::ranges::to<std::vector>();
        return std::unique_ptr<Mesh>(new Mesh(std::move(v), std::move(i)));
    }
    virtual std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const override;
private:
    using VerticesType = std::vector<Vertex>;
    using IndicesType = std::vector<glm::u32vec3>;
    Mesh(VerticesType&& vertices, IndicesType&& indices) : vertices(std::move(vertices)), indices(std::move(indices))
    {}
    VerticesType vertices;
    IndicesType indices;
};

}