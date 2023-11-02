#include <tracer/object.h>

#include <glm/gtc/constants.hpp>

namespace tracer
{

std::optional<float> Sphere::Intersect(const glm::vec3& orig, const glm::vec3& dir) const
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
    if (x1 < 0.0f)
        return x2;
    if (x2 < 0.0f)
        return x1;
    return min(x1, x2);
}

void Sphere::GetSurfaceData(const glm::vec3& point, SurfaceData& data) const
{
    SimpleMaterialObject::GetSurfaceData(point, data);

    using namespace glm;

    const float PI = pi<float>();

    glm::vec3 p = (point - origin) / radius;
    data.normal = p;
    data.texCoords = vec2(
        (atan2(p.z, p.x) / PI + 1.0f) / 2.0f,
        acos(p.y) / PI);
}

std::optional<float> Plane::Intersect(const glm::vec3& orig, const glm::vec3& dir) const
{
    using namespace glm;

    float denom = dot(normal, dir);
    if (denom < -1e-6f)
    {
        float t = dot(origin - orig, normal) / denom;
        if (t >= 0.0f)
            return t;
    }

    return std::nullopt;
}

void Plane::GetSurfaceData(const glm::vec3& point, SurfaceData& data) const
{
    SimpleMaterialObject::GetSurfaceData(point, data);

    data.normal = normal;
}

}