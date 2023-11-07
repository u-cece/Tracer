#include <tracer/mesh.h>

#include <optional>
#include <ranges>

#include "util.h"

namespace tracer
{

std::optional<float> Mesh::Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const
{
    using namespace glm;

    auto view = indices
        | std::views::transform([&, this](const u32vec3& i) -> std::optional<std::pair<float, SurfaceData>>
        {
            Vertex v0 = vertices.at(i.x);
            Vertex v1 = vertices.at(i.y);
            Vertex v2 = vertices.at(i.z);

            vec3 p0 = v0.pos;
            vec3 p1 = v1.pos;
            vec3 p2 = v2.pos;

            vec2 t0 = v0.texCoords;
            vec2 t1 = v1.texCoords;
            vec2 t2 = v2.texCoords;

            vec2 coords;
            std::optional<float> t = intersectTriangle(orig, dir, p0, p1, p2, coords);
            bool clockwise;
            if (!(clockwise = t.has_value()))
                t = intersectTriangleCounterClockwise(orig, dir, p0, p1, p2, coords);
            if (!t)
                return std::nullopt;
            
            SurfaceData data{};
            vec3 normal = clockwise ?
                cross(p2 - p0, p1 - p0) :
                cross(p1 - p0, p2 - p0);
            normal = normalize(normal);
            data.normal = normal;
            vec2 ab = t1 - t0;
            vec2 ac = t2 - t0;
            data.texCoords = t0 + ab * coords.x + ac * coords.y;
            return std::make_pair(t.value(), data);
        })
        | std::views::filter([](const std::optional<std::pair<float, SurfaceData>>& v) { return v.has_value(); })
        | std::views::transform([](const std::optional<std::pair<float, SurfaceData>>& v) { return v.value(); });
    
    if (std::ranges::distance(view.begin(), view.end()) == 0)
        return std::nullopt;

    auto [t, data] = std::ranges::min(view, {}, [](const std::pair<float, SurfaceData>& v)
    {
        return v.first;
    });
    surfaceData = data;
    surfaceData.material = GetMaterial();
    return t;
}

}