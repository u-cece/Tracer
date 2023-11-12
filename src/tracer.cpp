#include <tracer/tracer.h>

#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <print>
#include <random>
#include <ranges>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/color_space.hpp>

#include "util.h"

namespace tracer
{

static glm::vec3 calcLookVec(float yaw, float pitch)
{
    using namespace glm;

    vec3 camLookVec
    (
        sin(yaw) * cos(pitch),
        sin(pitch),
        cos(yaw) * cos(pitch)
    );
    return normalize(camLookVec);
}

static glm::vec3 calcToFrustumPlane(const glm::vec2& on2DFrustumPlane, const glm::vec3& lookVec, float fov)
{
    using namespace glm;

    // both are in world space
    vec3 camRightInWorld = normalize(
        cross(lookVec, vec3(0.0f, 1.0f, 0.0f)));
    vec3 camUpInWorld = normalize(
        cross(camRightInWorld, lookVec));
    
    vec3 toFrustumPlane =
        (on2DFrustumPlane.x * camRightInWorld + on2DFrustumPlane.y * camUpInWorld) // translate the point on the view frustum to a plane in world space
        * tan(fov / 2.0f) // scale the plane based on fov
        + lookVec; // push it in the direction the camera is facing
    
    return toFrustumPlane;
}

static glm::vec3 castRay(const glm::vec3& _Orig, const glm::vec3& _Dir, const Scene& scene, const TracerConfiguration& config, RNG& rng)
{
    using namespace glm;

    vec3 color(0.0f);
    vec3 through(1.0f);

    vec3 orig = _Orig;
    vec3 dir = _Dir;

    const Object* insideObj = nullptr;

    for (uint32_t i = 0; i < config.rayTrace.maxDepth; i++)
    {
        bool inside = insideObj != nullptr;
        
        HitResult hit{};
        scene.Trace(orig, dir, hit);

        if (!hit.valid)
        {
            color += through * config.rayTrace.environmentColor;
            break;
        }

        const SurfaceData& surface = hit.surfaceData;
        vec3 point = hit.point;
        vec3 normal = surface.normal;
        vec3 biasedP = point + normal * config.rayTrace.bias;

        const Material* material = hit.surfaceData.material;
        vec3 emissive;
        emissive = material->GetEmissivity(surface.texCoords);

        color += through * emissive;

        vec3 sample;
        material->Shade(rng, dir, normal, surface.texCoords, sample, through, inside);

        if (i >= config.rayTrace.minDepth)
        {
            // russian roulette
            float p = max(through.r, max(through.g, through.b));
            if (rand() > p)
            //if (rand() > p)
                break;
            
            through *= 1.0f / p;
        }
        
        orig = biasedP;
        dir = sample;
    }
    return color;
}

void Tracer::Render(Canvas& canvas, const Camera& camera, const Scene& scene)
{
    assert(canvas.GetChannelCount() == 3);

    using namespace glm;

    u32vec2 dim(canvas.GetWidth(), canvas.GetHeight());
    uint64_t nPixels = dim.x * dim.y;

    std::vector<std::thread> threadPool(config.system.nThreads);
    std::atomic_uint64_t pixel;

    auto callable = [&, this]
    {
        while (true)
        {
            uint64_t p = pixel.fetch_add(1, std::memory_order_relaxed);

            float aspect = static_cast<float>(dim.x) / static_cast<float>(dim.y);

            if (p >= nPixels)
                return;

            uint32_t i = p % dim.x;
            uint32_t j = static_cast<uint32_t>(p / dim.x);
            vec2 ndc = (vec2(i, j) / vec2(dim) - vec2(0.5f)) * 2.0f;
                ndc.y = -ndc.y; // flip the vertical axis;

            // viewport space projected on the view frustum plane
            // vec2 onFrustumPlane2D = aspect > 1.0f ?
            //     vec2(ndc.x * aspect, ndc.y         ) :
            //     vec2(ndc.x         , ndc.y / aspect);
            vec2 onFrustumPlane2D = aspect > 1.0f ?
                vec2(ndc.x         , ndc.y / aspect) :
                vec2(ndc.x * aspect, ndc.y         );

            vec3 camLookVec = calcLookVec(camera.yaw, camera.pitch);
            vec3 toFustumPlane = calcToFrustumPlane(onFrustumPlane2D, camLookVec, config.lens.fov);
            
            vec3 color(0.0f);
            vec3 accumColor(0.0f);

            vec3 axis1, axis2;
            createCoordSystemWithUpVec(camLookVec, axis1, axis2); // coord system of the defocus disk
            vec3 focusPoint = toFustumPlane * config.lens.focalPlaneDistance; // get the focus point on the focal plane by pushing the frustum plane out

            for (uint32_t _ = 0; _ < config.rayTrace.nRaysPerPixel; _++)
            {
                float r1 = rng.Uniform();
                float r2 = rng.Uniform();
                vec2 diskSample = samplePointOnDisk(r1, r2);
                vec3 defocused = (diskSample.x * axis1 + diskSample.y * axis2) * config.lens.defocusDiskRadius;
                
                vec3 rayColor = castRay(camera.pos + defocused, normalize(focusPoint - defocused), scene, config, rng);
                if (!std::isnan(rayColor.x) &&
                    !std::isnan(rayColor.y) &&
                    !std::isnan(rayColor.z))
                    accumColor += rayColor;
            }

            accumColor /= config.rayTrace.nRaysPerPixel;

            color = accumColor;

            for (uint32_t k = 0; k < 3; k++)
                canvas.Store(i, j, k, color[k]);
        }
    };

    for (auto& t : threadPool)
        t = std::thread(callable);

    auto startTime = std::chrono::steady_clock::now();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    uint64_t nPixelsCompleted;
    while ((nPixelsCompleted = pixel.load(std::memory_order_relaxed)) < nPixels)
    {
        std::chrono::duration<double, std::milli> duration = std::chrono::steady_clock::now() - startTime;
        auto timeRemaining = static_cast<double>(nPixels - nPixelsCompleted) / static_cast<double>(nPixelsCompleted) * duration;
        std::chrono::seconds remainingDuration = std::chrono::duration_cast<std::chrono::seconds>(timeRemaining);
        int64_t remainingSeconds = remainingDuration.count();
        int64_t remainingMinutes = remainingSeconds / 60;
        int64_t remainingHours = remainingMinutes / 60;
        uint64_t secondsDisp = remainingSeconds % 60;
        uint64_t minutesDisp = remainingMinutes % 60;
        uint64_t hoursDisp = remainingHours;

        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local{};
        localtime_s(&local, &time);
        std::chrono::hh_mm_ss formatter(std::chrono::hours(local.tm_hour) + std::chrono::minutes(local.tm_min) + std::chrono::seconds(local.tm_sec));
        std::println("[{:%T}]: {} out of {} pixels completed ({:.2f}%); Estimated time remaining: {:02}:{:02}:{:02}",
            formatter,
            nPixelsCompleted,
            nPixels,
            static_cast<double>(nPixelsCompleted) / static_cast<double>(nPixels) * 100.0,
            hoursDisp, minutesDisp, secondsDisp);

        std::this_thread::sleep_for(1s);
    }

    for (auto& t : threadPool)
        if (t.joinable())
            t.join();
}

}