#include <tracer/mesh.h>

#include "util.h"

namespace tracer
{

std::optional<float> Mesh::Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const
{
    using namespace glm;
    for (const auto& i : indices)
    {
        vec3 p0 = vertices.at(i.x).pos;
        vec3 p1 = vertices.at(i.y).pos;
        vec3 p2 = vertices.at(i.z).pos;

        std::optional<float> t = intersectTriangle(orig, dir, p0, p1, p2);
        bool clockwise;
        if (!(clockwise = t.has_value()))
            t = intersectTriangleCounterClockwise(orig, dir, p0, p1, p2);
        if (t)
        {
            vec3 normal = clockwise ?
                cross(p2 - p0, p1 - p0) :
                cross(p1 - p0, p2 - p0);
            normal = normalize(normal);
            surfaceData.normal = normal;
            surfaceData.material = GetMaterial();
            return t;
        }
    }

    return std::nullopt;
}

}