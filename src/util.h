#pragma once

#include <fstream>
#include <sstream>
#include <optional>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <tracer/rng.h>

namespace tracer
{

inline void createCoordSystemWithUpVec(const glm::vec3& up, glm::vec3& axis1, glm::vec3& axis2)
{
    using namespace glm;

    axis2 = normalize(
        abs(up.x) > abs(up.y) ?
            vec3(up.z, 0, -up.x) :
            vec3(0, -up.z, up.y));
    axis1 = cross(up, axis2);
}

inline glm::vec2 samplePointOnDisk(float r1, float r2)
{
    using namespace glm;

    float r = sqrt(r1);
    float theta = r2 * 2.0f * pi<float>();
    return vec2(r * cos(theta), r * sin(theta));
}

inline std::optional<float> intersectPlane(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& p, const glm::vec3& normal)
{
    using namespace glm;

    float denom = dot(normal, dir);
    if (denom > -1e-6f)
        return std::nullopt;

    float t = dot(p - orig, normal) / denom;
    if (t < 0.0f)
        return std::nullopt;
    
    return t;
}

inline bool insideOutsideTest(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& point, const glm::vec3& normal, glm::vec2& coords)
{
    using namespace glm;

    vec3 e0 = p1 - p0;
    vec3 e1 = p2 - p1;
    vec3 e2 = p0 - p2;

    vec3 c0 = point - p0;
    vec3 c1 = point - p1;
    vec3 c2 = point - p2;

    vec3 c0xe0 = cross(c0, e0);
    vec3 c1xe1 = cross(c1, e1);
    vec3 c2xe2 = cross(c2, e2);

    if (dot(normal, c0xe0) < 0.0f ||
        dot(normal, c1xe1) < 0.0f ||
        dot(normal, c2xe2) < 0.0f)
        return false;

    float area = length(cross(p2 - p0, p1 - p0));
    coords.x = length(c2xe2) / area;
    coords.y = length(c0xe0) / area;

    return true;
}

inline bool insideOutsideTestCounterClockwise(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& point, const glm::vec3& normal, glm::vec2& coords)
{
    using namespace glm;

    vec3 e0 = p1 - p0;
    vec3 e1 = p2 - p1;
    vec3 e2 = p0 - p2;

    vec3 c0 = point - p0;
    vec3 c1 = point - p1;
    vec3 c2 = point - p2;

    vec3 e0xc0 = cross(e0, c0);
    vec3 e1xc1 = cross(e1, c1);
    vec3 e2xc2 = cross(e2, c2);

    if (dot(normal, e0xc0) < 0.0f ||
        dot(normal, e1xc1) < 0.0f ||
        dot(normal, e2xc2) < 0.0f)
        return false;

    float area = length(cross(p2 - p0, p1 - p0));
    coords.x = length(e2xc2) / area;
    coords.y = length(e0xc0) / area;

    return true;
}

// assumes clock-wise
inline std::optional<float> intersectTriangle(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec2& coords)
{
    using namespace glm;

    vec3 normal = normalize(cross(p2 - p0, p1 - p0));

    std::optional<float> planeIntersection = intersectPlane(orig, dir, p0, normal);
    if (!planeIntersection)
        return std::nullopt;
    
    float t = planeIntersection.value();

    vec3 point = orig + t * dir;
    return insideOutsideTest(p0, p1, p2, point, normal, coords) ?
        std::optional<float>(t) :
        std::nullopt;
}

inline std::optional<float> intersectTriangleCounterClockwise(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec2& coords)
{
    using namespace glm;

    vec3 normal = normalize(cross(p1 - p0, p2 - p0));

    std::optional<float> planeIntersection = intersectPlane(orig, dir, p0, normal);
    if (!planeIntersection)
        return std::nullopt;
    
    float t = planeIntersection.value();

    vec3 point = orig + t * dir;
    return insideOutsideTestCounterClockwise(p0, p1, p2, point, normal, coords) ?
        std::optional<float>(t) :
        std::nullopt;
}

inline std::optional<float> intersectTriangleMT(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec2& coords)
{
    using namespace glm;

    vec3 ab = p2 - p0;
    vec3 ac = p1 - p0;
    vec3 pVec = cross(dir, ac);

    float det = dot(ab, pVec);
    if (det < 1e-6f)
        return std::nullopt;
    
    float invDet = 1.0f / det;

    vec3 tVec = orig - p0;
    float u = dot(tVec, pVec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return std::nullopt;
    
    vec3 qVec = cross(tVec, ab);
    float v = dot(dir, qVec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return std::nullopt;
    
    coords.x = v;
    coords.y = u;

    return dot(ac, qVec) * invDet;
}

inline std::optional<float> intersectTriangleCounterClockwiseMT(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec2& coords)
{
    using namespace glm;

    vec3 ab = p1 - p0;
    vec3 ac = p2 - p0;
    vec3 pVec = cross(dir, ac);

    float det = dot(ab, pVec);
    if (det < 1e-6f)
        return std::nullopt;
    
    float invDet = 1.0f / det;

    vec3 tVec = orig - p0;
    float u = dot(tVec, pVec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return std::nullopt;
    
    vec3 qVec = cross(tVec, ab);
    float v = dot(dir, qVec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return std::nullopt;
    
    coords.x = u;
    coords.y = v;

    return dot(ac, qVec) * invDet;
}

inline void sampleBarycentricUniform(RNG& rng, glm::vec2& coords)
{
    using namespace glm;

    float r1 = rng.Uniform();
    float r2 = rng.Uniform();

    float sqrtR1 = sqrt(r1);
    coords.x = 1.0f - sqrtR1;
    coords.y = r2 * sqrtR1;
}

inline void sampleTriangleUniform(RNG& rng, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec3& sample, float& pdf)
{
    using namespace glm;

    vec2 coords;
    sampleBarycentricUniform(rng, coords);
    
    vec3 ab = p1 - p0;
    vec3 ac = p2 - p0;

    sample = (ab * coords.x + ac * coords.y) / 2.0f + p0;

    float area = length(cross(ab, ac)) / 2.0f;
    pdf = 1.0f / area;
}

inline std::string readTextFile(const std::string& path)
{
    std::ifstream stream(path);
    std::ostringstream oss;
    oss << stream.rdbuf();
    return oss.str();
}

}