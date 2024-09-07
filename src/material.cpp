#include <tracer/material.h>

#include <glm/gtc/constants.hpp>

#include "util.h"

namespace tracer
{

bool ReflectiveMaterial::Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, float& currentIor, bool& isInside) const
{
    using namespace glm;

    float pdf;
    vec3 brdf;
    SampleAndCalcBrdf(rng, rayDir, normal, texCoords, lightSample, lightSamplePdfFunc, wi, pdf, brdf, currentIor);
    float cosTerm = dot(wi, normal);
    if (cosTerm < 0.0f)
        cosTerm = 0.0f;
    attenuation *= brdf * cosTerm / pdf;

    return true;
}

void SimpleDiffuseMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf, float& currentIor) const
{
    using namespace glm;

    if (lightSample)
    {
        float p = 0.5f;
        float cosinePdf;
        if (rng.Uniform() < p)
        {
            sample = lightSample.value();
            cosinePdf = getCosinePdf(transformWorldSampleToLocal(normal, sample));
        }
        else
        {
            vec3 localSample;
            generateCosine(rng, localSample, cosinePdf);
            sample = transformLocalSampleToWorld(normal, localSample);
        }
        float lightPdf = lightSamplePdfFunc(sample);

        pdf = cosinePdf * (1.0f - p) + lightPdf * p;
    }
    else
    {
        vec3 localSample;
        float generatedPdf;
        generateCosine(rng, localSample, generatedPdf);
        vec3 generatedSample = transformLocalSampleToWorld(normal, localSample);
        sample = generatedSample;
        pdf = generatedPdf;
    }

    brdf = texture->SampleOptional(texCoords) / pi<float>();
}

void SimpleMirrorMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf, float& currentIor) const
{
    using namespace glm;

    sample = reflect(rayDir, normal);

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

bool ExposedMediumMaterial::Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, float& currentIor, bool& isInside) const
{
    using namespace glm;

    float n;
    if (isInside) // going out
        n = 1.0f / mediumIor;
    else // going in
        n = mediumIor;

    float f = fresnel(n, dot(-rayDir, normal));

    float cosTheta = dot(-rayDir, normal);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    if (rng.Uniform() < f || n * sinTheta > 1.0f)
    {
        wi = reflect(rayDir, normal);
        attenuation *= 1.0f;
        return true;
    }

    if (isInside)
    {
        isInside = false;
        currentIor = 1.0f;
    }
    else
    {
        isInside = true;
        currentIor = mediumIor;
    }
    wi = refract(rayDir, normal, n);
    attenuation *= 1.0f;

    return true;
}

void SpecularCoatedMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf, float& currentIor) const
{
    using namespace glm;

    vec3 localSample;
    generateUniform(rng, localSample, pdf);
    sample = transformLocalSampleToWorld(normal, localSample);

    vec3 half = normalize(sample - rayDir);

    vec3 albedo = albedoTexture->SampleOptional(texCoords);
    float alpha = alphaTexture->SampleOptional(texCoords).r;

    float d = ndfGgx(alpha, normal, half);

    float g = gemetricShadowing(alpha, normal, half, -rayDir) * gemetricShadowing(alpha, normal, half, sample);

    float f = fresnel(ior / currentIor, dot(-rayDir, half));

    float specular = d * g * f / (4.0f * dot(-rayDir, normal) * dot(sample, normal));

    brdf = (1.0f - f) * albedo / pi<float>() + specular;
}

void PerfectSpecularCoatedMaterial::SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf, float& currentIor) const
{
    using namespace glm;

    float f = fresnel(ior / currentIor, dot(-rayDir, normal));

    if (rng.Uniform() < f)
    {
        sample = reflect(rayDir, normal);
        pdf = 1.0f;
        brdf = vec3(1.0f) / dot(sample, normal);
        return;
    }
    
    if (lightSample)
    {
        float p = 0.5f;
        float cosinePdf;
        if (rng.Uniform() < p)
        {
            sample = lightSample.value();
            cosinePdf = getCosinePdf(transformWorldSampleToLocal(normal, sample));
        }
        else
        {
            vec3 localSample;
            generateCosine(rng, localSample, cosinePdf);
            sample = transformLocalSampleToWorld(normal, localSample);
        }
        float lightPdf = lightSamplePdfFunc(sample);

        pdf = cosinePdf * (1.0f - p) + lightPdf * p;
    }
    else
    {
        vec3 localSample;
        float cosinePdf;
        generateCosine(rng, localSample, cosinePdf);
        vec3 cosineSample = transformLocalSampleToWorld(normal, localSample);
        sample = cosineSample;
        pdf = cosinePdf;
    }

    brdf = albedoTexture->SampleOptional(texCoords) / pi<float>();
}

}