#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

}