#pragma once

#include <algorithm>
#include <optional>

#include <glm/glm.hpp>

namespace tracer
{

class AABB
{
public:
    AABB() = default;
    AABB(const glm::vec3& a, const glm::vec3& b)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            min[i] = std::min(a[i], b[i]);
            max[i] = std::max(a[i], b[i]);
        }
    }
    AABB(const AABB& a, const AABB& b)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            min[i] = std::min(a.min[i], b.min[i]);
            max[i] = std::max(a.max[i], b.max[i]);
        }
    }
    void Grow(const glm::vec3& pos)
    {
        for (uint32_t i = 0; i < 3; i++)    
        {
            min[i] = std::min(min[i], pos[i]);
            max[i] = std::max(max[i], pos[i]);
        }
    }
    glm::vec3 GetSize() const
    {
        return max - min;
    }
    glm::vec3 GetCenter() const
    {
        return (min + max) / 2.0f;
    }
    glm::vec3 GetMin() const
    {
        return min;
    }
    glm::vec3 GetMax() const
    {
        return max;
    }
    bool IsInside(const glm::vec3& p) const
    {
        return
            p.x >= min.x && p.x <= max.x &&
            p.y >= min.y && p.y <= max.y &&
            p.z >= min.z && p.z <= max.z;
    }
    std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir) const
    {
        glm::vec3 tMin = (min - orig) / dir;
        glm::vec3 tMax = (max - orig) / dir;
        glm::vec3 t1 = glm::min(tMin, tMax);
        glm::vec3 t2 = glm::max(tMin, tMax);
        float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
        float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
        if (tNear > tFar)
            return std::nullopt;
        if (tNear < 0.0f)
            return std::nullopt;
        return tNear;
    }
private:
    glm::vec3 min, max;
};

}