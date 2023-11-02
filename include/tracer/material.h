#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "sampler.h"
#include "rng.h"

namespace tracer
{

class Material
{
public:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const = 0;
    virtual glm::vec3 GetEmissivity(const glm::vec2& texCoords) const  { return glm::vec3(0.0f); }
    virtual glm::vec3 GetFallbackEmissivity() const { return glm::vec3(0.0f); }
};

class SimpleDiffuseMaterial : public Material
{
public:
    SimpleDiffuseMaterial(const glm::vec3& albedo);
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    glm::vec3 albedo;
};

class SimpleEmissiveMaterial : public SimpleDiffuseMaterial
{
public:
    SimpleEmissiveMaterial(const glm::vec3& albedo, const glm::vec3& emissivity) : SimpleDiffuseMaterial(albedo), emissivity(emissivity) {}
    virtual glm::vec3 GetEmissivity(const glm::vec2& texCoords) const { return emissivity; }
    virtual glm::vec3 GetFallbackEmissivity() const { return emissivity; }
private:
    glm::vec3 emissivity;
};

class SimpleReflectiveMaterial : public Material
{
public:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
};

class CookTorranceMaterial : public Material
{
public:
    CookTorranceMaterial(const glm::vec3& albedo, float alpha, float ior) : albedo(albedo), alpha{alpha}, ior{ior} {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    glm::vec3 albedo;
    float alpha;
    float ior;
};

}