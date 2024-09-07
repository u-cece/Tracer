#pragma once

#include <glm/glm.hpp>

#include "camera.h"
#include "canvas.h"
#include "rng.h"
#include "scene.h"

namespace tracer
{

struct SystemConfiguration
{
    uint32_t nThreads = 4u;
};

struct RayTraceConfiguration
{
    float bias = 1e-4f;
    uint32_t nMinBounces = 3u;
    uint32_t nMaxBounces = 16u;
    uint32_t nSamplesPerPixel = 16u;
};

struct TracerConfiguration
{
    uint32_t nThreads = 4u;

    uint32_t width;
    uint32_t height;

    float bias = 1e-4f;
    uint32_t nMinBounces = 3u;
    uint32_t nMaxBounces = 16u;
    uint32_t nSamplesPerPixel = 16u;
};

class Tracer
{
public:
    Tracer(const TracerConfiguration& config) : config(config)
    {}
    Tracer() : config{}
    {}
    void Render(Canvas& canvas, const Scene& scene);
private:
    TracerConfiguration config;
    RNG rng;
};

}