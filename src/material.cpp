#include <tracer/material.h>

#include <glm/gtc/constants.hpp>

#include "util.h"

namespace tracer
{

void SimpleReflectiveMaterial::Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& wi, glm::vec3& attenuation, bool& inside) const
{
    using namespace glm;

    float pdf;
    vec3 brdf;
    SampleAndCalcBrdf(rng, rayDir, normal, texCoords, wi, pdf, brdf);
    attenuation *= brdf * dot(wi, normal) / pdf;
}

void SimpleDiffuseMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    vec3 localSample;
    samplers::generateCosine(rng, localSample, pdf);
    sample = transformSampleToWorld(normal, localSample);

    brdf = texture->SampleOptional(texCoords) / pi<float>();
}

glm::vec3 calcMirroredRay(const glm::vec3& in, const glm::vec3& normal)
{
    using namespace glm;
    vec3 b = dot(-in, normal) * normal;
    return in + 2.0f * b;
}

void SimpleMirrorMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    sample = calcMirroredRay(rayDir, normal);

    pdf = 1.0f;

    brdf = vec3(1.0f) / dot(sample, normal);
}

static float ndfGgx(float alpha, const glm::vec3& normal, const glm::vec3& half)
{
    using namespace glm;

    float cosine = dot(normal, half);
    float d = 0.0f;
    if (cosine <= 0.0f)
        return 0;

    float in = cosine * cosine * (alpha * alpha - 1.0f) + 1.0f;
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

void SpecularCoatedMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    vec3 localSample;
    samplers::generateUniform(rng, localSample, pdf);
    sample = transformSampleToWorld(normal, localSample);

    vec3 half = normalize(sample - rayDir);

    vec3 albedo = albedoTexture->SampleOptional(texCoords);
    float alpha = alphaTexture->SampleOptional(texCoords).r;

    float d = ndfGgx(alpha, normal, half);

    float g = gemetricShadowing(alpha, normal, half, -rayDir) * gemetricShadowing(alpha, normal, half, sample);

    float f = fresnel(ior, dot(-rayDir, half));

    float specular = d * g * f / (4.0f * dot(-rayDir, normal) * dot(sample, normal));

    brdf = (1.0f - f) * albedo / pi<float>() + specular;
}

void PerfectSpecularCoatedMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const
{
    using namespace glm;

    float f = fresnel(ior, dot(-rayDir, normal));

    if (rng.Uniform() < f)
    {
        sample = calcMirroredRay(rayDir, normal);
        pdf = 1.0f;
        brdf = vec3(1.0f) / dot(sample, normal);
        return;
    }
    
    samplers::generateCosine(rng, sample, pdf);
    sample = transformSampleToWorld(normal, sample);

    brdf = albedo / pi<float>();
}

}