#pragma once

#include <algorithm>

#include <glm/glm.hpp>

namespace tracer
{

class AABB
{
public:
    AABB(const glm::vec3& a, const glm::vec3& b)
    {
        for (uint32_t i = 0; i < 3; i++)
            from[i] = std::min(a[i], b[i]);
        for (uint32_t i = 0; i < 3; i++)
            to[i] = std::max(a[i], b[i]);
        size = to - from;
    }
    bool Intersect(const glm::vec3& orig, const glm::vec3& dir) const
    {
        using namespace glm;
        vec3 tMin = (from - orig) / dir;
        vec3 tMax = (to - orig) / dir;
        vec3 t1 = min(tMin, tMax);
        vec3 t2 = max(tMin, tMax);
        float tNear = max(max(t1.x, t1.y), t1.z);
        float tFar = min(min(t2.x, t2.y), t2.z);
        return tNear <= tFar;
    }
private:
    glm::vec3 from, to;
    glm::vec3 size;
};

}