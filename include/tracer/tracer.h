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
    glm::vec3 environmentColor{0.0f};
    uint32_t nMinBounces = 3u;
    uint32_t nMaxBounces = 16u;
    uint32_t nRaysPerPixel = 16u;
};

struct LensConfiguration
{
    float fov = glm::radians(90.0f);
    float defocusDiskRadius = 0.0f;
    float focalPlaneDistance = 1.0f;

    static void fromLensParams(LensConfiguration& config, float focalLength, float sensorSize, float fStop, float focusPoint)
    {
        using namespace glm;

        config.fov = 2.0f * atan(sensorSize / (2.0f * focalLength));
        config.defocusDiskRadius = focalLength / fStop / 2.0f;
        config.focalPlaneDistance = focusPoint;
    }
};

struct TracerConfiguration
{
    SystemConfiguration system{};
    RayTraceConfiguration rayTrace{};
    LensConfiguration lens{};
};

class Tracer
{
public:
    Tracer(const TracerConfiguration& config) : config(config)
    {}
    Tracer() : config{}
    {}
    void Render(Canvas& canvas, const Camera& camera, const Scene& scene);
private:
    TracerConfiguration config;
    RNG rng;
};

}