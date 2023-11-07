#pragma once

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <span>
#include <string_view>

namespace tracer
{

class Canvas
{
public:
    Canvas(uint32_t width, uint32_t height, uint32_t channel)
        :
        width(width), height(height), channelCount(channel),
        data(std::make_unique<uint8_t[]>(getDataSize()))
    {}
    void Store(uint32_t w, uint32_t h, uint32_t channel, uint8_t byte)
    {
        data[getIndex(w, h, channel)] = byte;
    }
    void Store(uint32_t w, uint32_t h, uint32_t channel, float v)
    {
        uint8_t byte = static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        Store(w, h, channel, byte);
    }
    uint8_t LoadByte(uint32_t w, uint32_t h, uint32_t channel) const
    {
        return data[getIndex(w, h, channel)];
    }
    float LoadFloat(uint32_t w, uint32_t h, uint32_t channel) const
    {
        return static_cast<float>(LoadByte(w, h, channel)) / 255.0f;
    }
    uint32_t GetWidth() const { return width; }
    uint32_t GetHeight() const { return height; }
    uint32_t GetChannelCount() const { return channelCount; }
    std::span<const uint8_t> GetData() const
    {
        return std::span<const uint8_t>(data.get(), getDataSize());
    }
    void SaveToPNG(std::string_view file) const;
private:
    size_t getDataSize() const { return width * height * channelCount; }
    size_t getIndex(uint32_t w, uint32_t h, uint32_t channel) const
    {
        assert(w < width && h < height);
        return (h * width + w) * channelCount + channel;
    }
    const uint32_t width, height, channelCount;
    std::unique_ptr<uint8_t[]> data;
};

}