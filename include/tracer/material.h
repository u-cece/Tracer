#pragma once

#include <memory>
#include <optional>

#include <glm/glm.hpp>

#include "rng.h"
#include "sampler.h"
#include "texture.h"

namespace tracer
{

class Material
{
public:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const = 0;
    virtual glm::vec3 GetEmissivity(const std::optional<glm::vec2>& texCoords) const  { return glm::vec3(0.0f); }
    virtual ~Material() {}
};

class SimpleDiffuseMaterial : public Material
{
public:
    SimpleDiffuseMaterial(const std::shared_ptr<Texture>& texture)
    {
        this->texture = texture;
    }
    SimpleDiffuseMaterial(const glm::vec3& albedo) : SimpleDiffuseMaterial(std::make_shared<SimpleGradientTexture>(albedo))
    {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    std::shared_ptr<Texture> texture;
};

class SimpleEmissiveMaterial : public SimpleDiffuseMaterial
{
public:
    SimpleEmissiveMaterial(const std::shared_ptr<Texture>& albedo, const std::shared_ptr<Texture>& emissivity, float multiplier = 1.0f)
        : SimpleDiffuseMaterial(albedo), texture(emissivity), multiplier(multiplier)
    {}
    SimpleEmissiveMaterial(const glm::vec3& albedo, const glm::vec3& emissivity)
        : SimpleDiffuseMaterial(albedo), texture(std::make_shared<SimpleGradientTexture>(emissivity))
    {}
    virtual glm::vec3 GetEmissivity(const std::optional<glm::vec2>& texCoords) const override { return multiplier * texture->SampleOptional(texCoords); }
private:
    std::shared_ptr<Texture> texture;
    float multiplier;
};

class SimpleReflectiveMaterial : public Material
{
public:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
};

class SpecularCoatedMaterial : public Material
{
public:
    SpecularCoatedMaterial(const std::shared_ptr<Texture>& albedo, const std::shared_ptr<Texture>& alpha, const std::shared_ptr<Texture>& ior)
        : albedoTexture(albedo), alphaTexture(alpha), iorTexture(ior)
    {}
    SpecularCoatedMaterial(const glm::vec3& albedo, float alpha, float ior)
        : albedoTexture(std::make_shared<SimpleGradientTexture>(albedo)), alphaTexture(std::make_shared<SimpleGradientTexture>(glm::vec3(alpha))), iorTexture(std::make_shared<SimpleGradientTexture>(glm::vec3(ior)))
    {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    std::shared_ptr<Texture> albedoTexture;
    std::shared_ptr<Texture> alphaTexture;
    std::shared_ptr<Texture> iorTexture;
};

class PerfectSpecularCoatedMaterial : public Material
{
public:
    PerfectSpecularCoatedMaterial(const glm::vec3& albedo, float ior) : albedo(albedo), ior{ior} {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    glm::vec3 albedo;
    float ior;
};

}