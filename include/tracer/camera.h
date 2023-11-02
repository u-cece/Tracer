#pragma once

#include <glm/glm.hpp>

namespace tracer
{

struct Camera
{
    glm::vec3 pos;
    float yaw, pitch;
};

}