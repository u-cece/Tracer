#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "rng.h"

namespace tracer
{

struct EmissionSample
{
    glm::vec3 sample;
};

class EmissionProfile
{
public:
    virtual std::optional<EmissionSample> Sample(RNG& rng, const glm::vec3& orig, const glm::vec3& pNorm) const = 0;
    virtual float GetPdf(const glm::vec3& orig, const glm::vec3& dir) const = 0;
};

}