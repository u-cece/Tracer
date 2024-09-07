#include <tracer/texture.h>

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace tracer
{

ImageTexture::ImageTexture(std::string_view file)
{
    std::string str(file);
    int x, y, n;
    unsigned char* bytes = stbi_load(str.data(), &x, &y, &n, 3);
    width = x;
    height = y;
    uint32_t pixelCount = x * y;
    uint32_t size = pixelCount * 3;
    data = std::make_unique<glm::u8vec3[]>(pixelCount);
    memcpy_s(data.get(), size, bytes, size);
    stbi_image_free(bytes);
}

glm::vec3 ImageTexture::Sample(const glm::vec2& uv) const
{
    uint32_t w = static_cast<uint32_t>(uv.x * width);
    uint32_t h = static_cast<uint32_t>((1.0f - uv.y) * height);
    w = std::clamp(w, 0u, width - 1);
    h = std::clamp(h, 0u, height - 1);
    return toFloats(load(w, h));
}

}