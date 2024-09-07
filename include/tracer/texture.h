#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <glm/glm.hpp>

namespace tracer
{

class Texture
{
public:
    glm::vec3 SampleOptional(const std::optional<glm::vec2>& uv)
    {
        if (uv)
            return Sample(uv.value());
        return Fallback();
    }
    virtual glm::vec3 Sample(const glm::vec2& uv) const = 0;
    virtual glm::vec3 Fallback() const
    {
        return glm::vec3(0.0f);
    }
    virtual ~Texture() {}
};

class SimpleGradientTexture : public Texture
{
public:
    SimpleGradientTexture(const glm::vec3& color) : SimpleGradientTexture(color, color, color, color)
    {}
    SimpleGradientTexture(const glm::vec3& topL, const glm::vec3& topR, const glm::vec3& botR, const glm::vec3& botL)
        : topL(topL), topR(topR), botR(botR), botL(botL), avg((topL + topR + botR + botL) / 4.0f)
    {}
    glm::vec3 Sample(const glm::vec2& uv) const override
    {
        using namespace glm;
        vec3 bot = uv.s * botR + (1.0f - uv.s) * botL;
        vec3 top = uv.s * topR + (1.0f - uv.s) * topL;
        return uv.t * top + (1.0f - uv.t) * bot;
    }
    glm::vec3 Fallback() const override
    {
        return avg;
    }
private:
    glm::vec3 topL, topR, botR, botL;
    glm::vec3 avg;
};

class ImageTexture : public Texture
{
public:
    ImageTexture(std::string_view file);
    glm::vec3 Sample(const glm::vec2& uv) const;
private:
    static glm::vec3 toFloats(const glm::u8vec3& bytes)
    {
        return glm::vec3(bytes) / 255.0f;
    }
    glm::u8vec3 load(uint32_t u, uint32_t v) const
    {
        return data[getIndex(u, v)];
    }
    size_t getIndex(uint32_t u, uint32_t v) const
    {
        assert(u < width && v < height);
        return (v * width + u);
    }
    uint32_t width, height;
    std::unique_ptr<glm::u8vec3[]> data;
};

}