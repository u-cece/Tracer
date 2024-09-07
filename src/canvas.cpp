#include <tracer/canvas.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace tracer
{

void Canvas::SaveToPNG(std::string_view file) const
{
    assert(channelCount <= 4 && channelCount >= 3);
    stbi_write_png("out.png", width, height, channelCount, data.get(), 0);
}

}