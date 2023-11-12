#pragma once

#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "material.h"
#include "object.h"

namespace tracer
{

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texCoords;
};

struct Triad
{
    glm::u32vec3 indices;
    const Material* material;
};

enum class CullMode // front = clockwise, back = counter-clockwise
{
    None, Front, Back
};

class Mesh : public Object
{
public:
    static std::unique_ptr<Mesh> Create(std::string_view jsonStr);
    virtual std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const override;
private:
    enum class PrimitiveType
    {
        Solid, Translucent
    };

    Mesh() {}

    std::vector<std::unique_ptr<Material>> materialHolder;
    std::vector<Triad> triads;
    std::vector<Vertex> vertices;
    CullMode cullMode;
};

}