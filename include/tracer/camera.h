#pragma once

#include <glm/glm.hpp>

namespace tracer
{

struct Lens
{
    float fov;
    float defocusDiskRadius;
    float focalPointDistance;
};

struct Camera
{
    glm::vec3 pos;
    glm::vec3 dir;
    Lens lens;
};

}