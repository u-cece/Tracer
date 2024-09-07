#include <tracer/object.h>

#include <glm/gtc/constants.hpp>

#include "util.h"

namespace tracer
{

std::optional<float> Sphere::Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const
{
    using namespace glm;

    vec3 l = orig - origin;
    float b = 2.0f * dot(dir, l);
    float c = dot(l, l) - radiusSquared;

    float delta = b * b - 4.0f * c;

    if (delta < 0.0f)
        return std::nullopt;
    
    float q = (b > 0.0f) ?
        -0.5f * (b + sqrt(delta)) :
        -0.5f * (b - sqrt(delta));
    float x1 = q;
    float x2 = c / q;

    if (x1 < 0.0f && x2 < 0.0f)
        return std::nullopt;

    float distance;
    if (x1 < 0.0f)
        distance = x2;
    else if (x2 < 0.0f)
        distance = x1;
    else
        distance = min(x1, x2);

    surfaceData.material = GetMaterial();

    glm::vec3 p = (orig + distance * dir - origin) / radius;
    surfaceData.normal = p;
    surfaceData.texCoords = vec2(
        (atan2(p.z, p.x) / pi<float>() + 1.0f) / 2.0f,
        acos(p.y) / pi<float>());
    
    return distance;
}

std::optional<float> Plane::Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const
{
    using namespace glm;

    std::optional<float> t = intersectPlane(orig, dir, origin, normal);
    if (!t)
        return std::nullopt;

    surfaceData.material = GetMaterial();
    surfaceData.normal = normal;

    return t;
}

}