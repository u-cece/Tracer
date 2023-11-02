#include <tracer/material.h>

#include <glm/gtc/constants.hpp>

#include "util.h"

namespace tracer
{

SimpleDiffuseMaterial::SimpleDiffuseMaterial(const glm::vec3& albedo)
    :
    albedo(albedo)
{
}

void SimpleDiffuseMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    vec3 localSample;
    samplers::generateCosine(rng, localSample, pdf);
    sample = transformSampleToWorld(normal, localSample);

    brdf = albedo / pi<float>();
}

void SimpleReflectiveMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    vec3 b = dot(-rayDir, normal) * normal;
    sample = rayDir + 2.0f * b;

    pdf = 1.0f;

    brdf = vec3(1.0f);
}

static float ndfGgx(float alpha, const glm::vec3& normal, const glm::vec3& half)
{
    using namespace glm;

    float nh = dot(normal, half);
    float d = 0.0f;
    if (nh <= 0.0f)
        return 0;

    float in = nh * nh * (alpha * alpha - 1.0f) + 1.0f;
    d = alpha * alpha / (pi<float>() * in * in);

    return d;
}

static float gemetricShadowing(float alpha, const glm::vec3& normal, const glm::vec3& half, const glm::vec3& d)
{
    using namespace glm;

    float dn = dot(d, normal);
    float dn2 = dn * dn;
    float g = 0.0f;
    if (dot(d, half) / dn <= 0.0f)
        return 0.0f;

    g = 2.0f / (1.0f + sqrt(1.0f + alpha * alpha * ((1.0f - dn2) / dn2)));

    return g;
}

static float schlickApprox(float n, float cosine)
{
    float r_0 = (1.0f - n) / (1.0f + n);
    r_0 *= r_0;
    float r = r_0 + (1.0f - r_0) * pow(1.0f - cosine, 5.0f);
    return r;
}

static float fresnel(float n, float cosine)
{
    float recN2 = 1.0f / (n * n);
    float sine2 = 1.0f - cosine * cosine;
    float sqrtComp = sqrt(1.0f - recN2 * sine2);
    float r_s = abs((cosine - n * sqrtComp) / (cosine + n * sqrtComp));
    r_s *= r_s;
    float r_p = abs((sqrtComp - n * cosine) / (sqrtComp + n * cosine));
    r_p *= r_p;
    return (r_s + r_p) / 2.0f;
}

void CookTorranceMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    vec3 localSample;
    samplers::generateUniform(rng, localSample, pdf);
    sample = transformSampleToWorld(normal, localSample);

    vec3 half = normalize(sample - rayDir);

    float d = ndfGgx(alpha, normal, half);

    float g = gemetricShadowing(alpha, normal, half, -rayDir) * gemetricShadowing(alpha, normal, half, sample);

    float f = fresnel(ior, dot(-rayDir, half));

    float specular = d * g * f / (4.0f * dot(-rayDir, normal) * dot(sample, normal));

    brdf = (1.0f - f) * albedo / pi<float>() + specular;
}

}