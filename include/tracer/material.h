#pragma once

#include <functional>
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
    // to-do: return a boolean to indicate whether it should generate further rays
    virtual bool Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, bool& inside) const = 0;
    virtual glm::vec3 GetEmissivity(const std::optional<glm::vec2>& texCoords) const { return glm::vec3(0.0f); }
    virtual bool IsEmissive() const { return false; }
    virtual ~Material() {}
};

class DebugMaterial : public Material
{
    virtual bool Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, bool& inside) const override
    {
        return false;
    }
    virtual glm::vec3 GetEmissivity(const std::optional<glm::vec2>& texCoords) const override { return glm::vec3(1.0f); }
};

class SimpleReflectiveMaterial : public Material
{
public:
    virtual bool Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, bool& inside) const override;
protected:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf) const = 0;
};

class SimpleDiffuseMaterial : public SimpleReflectiveMaterial
{
public:
    SimpleDiffuseMaterial(const std::shared_ptr<Texture>& texture)
    {
        this->texture = texture;
    }
    SimpleDiffuseMaterial(const glm::vec3& albedo) : SimpleDiffuseMaterial(std::make_shared<SimpleGradientTexture>(albedo))
    {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    std::shared_ptr<Texture> texture;
};

// black-body light source
class SimpleEmissiveMaterial : public Material
{
public:
    SimpleEmissiveMaterial(const std::shared_ptr<Texture>& emissivity, float multiplier = 1.0f)
        : texture(emissivity), multiplier(multiplier)
    {}
    SimpleEmissiveMaterial(const glm::vec3& albedo, const glm::vec3& emissivity)
        : texture(std::make_shared<SimpleGradientTexture>(emissivity))
    {}
    virtual bool Shade(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& wi, glm::vec3& attenuation, bool& inside) const override
    {
        return false;
    }
    virtual glm::vec3 GetEmissivity(const std::optional<glm::vec2>& texCoords) const override { return multiplier * texture->SampleOptional(texCoords); }
    virtual bool IsEmissive() const override { return true; }
private:
    std::shared_ptr<Texture> texture;
    float multiplier;
};

class SimpleMirrorMaterial : public SimpleReflectiveMaterial
{
public:
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
};

class SpecularCoatedMaterial : public SimpleReflectiveMaterial
{
public:
    SpecularCoatedMaterial(const std::shared_ptr<Texture>& albedo, const std::shared_ptr<Texture>& alpha, float ior)
        : albedoTexture(albedo), alphaTexture(alpha), ior(ior)
    {}
    SpecularCoatedMaterial(const glm::vec3& albedo, float alpha, float ior)
        : albedoTexture(std::make_shared<SimpleGradientTexture>(albedo)), alphaTexture(std::make_shared<SimpleGradientTexture>(glm::vec3(alpha))), ior(ior)
    {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    std::shared_ptr<Texture> albedoTexture;
    std::shared_ptr<Texture> alphaTexture;
    float ior;
};

class PerfectSpecularCoatedMaterial : public SimpleReflectiveMaterial
{
public:
    PerfectSpecularCoatedMaterial(const glm::vec3& albedo, float ior) : albedo(albedo), ior{ior} {}
    virtual void SampleAndCalcBrdf(RNG& rng, const glm::vec3& rayDir, const glm::vec3& normal, const std::optional<glm::vec2>& texCoords, const std::optional<glm::vec3>& lightSample, const std::function<float(const glm::vec3&)>& lightSamplePdfFunc, glm::vec3& sample, float& pdf, glm::vec3& brdf) const override;
private:
    glm::vec3 albedo;
    float ior;
};

}