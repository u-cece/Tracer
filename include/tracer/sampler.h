#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "rng.h"

namespace tracer
{

glm::vec3 transformLocalSampleToWorld(const glm::vec3& normal, const glm::vec3& sample);

glm::vec3 transformWorldSampleToLocal(const glm::vec3& normal, const glm::vec3& sample);

inline void generateUniform(RNG& rng, glm::vec3& sample, float& pdf)
{
    using namespace glm;

    float r1 = rng.Uniform();
    float r2 = rng.Uniform();

    float sinTheta = sqrt(1.0f - r1 * r1);
    float phi = 2.0f * r2 * pi<float>();
    sample = normalize(vec3(
        sinTheta * cos(phi),
        r1,
        sinTheta * sin(phi)
    ));
    pdf = 1.0f / (2.0f * pi<float>());
}

inline void generateCosine(RNG& rng, glm::vec3& sample, float& pdf)
{
    using namespace glm;

    float r1 = rng.Uniform();
    float r2 = rng.Uniform();

    float cosTheta = sqrt(1.0f - r1);
    float sinTheta = sqrt(r1);
    float phi = 2.0f * pi<float>() * r2;
    sample = normalize(vec3(
        sinTheta * cos(phi),
        cosTheta,
        sinTheta * sin(phi)
    ));
    pdf = cosTheta / pi<float>();
}

inline float getCosinePdf(const glm::vec3& sample)
{
    return sample.y / glm::pi<float>();
}

inline void generateGgx(RNG& rng, glm::vec3& sample, float& pdf)
{
    
}

}