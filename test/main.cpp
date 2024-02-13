#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <fmt/core.h>
#include <ranges>

#include <glm/gtc/matrix_transform.hpp>

#include <tracer/bvh.h>
#include <tracer/camera.h>
#include <tracer/canvas.h>
#include <tracer/mesh.h>
#include <tracer/scene.h>
#include <tracer/tracer.h>

int main()
{
    using namespace tracer;

    std::vector<std::unique_ptr<Object>> objects;
    
    Camera camera{};
    // camera.yaw = glm::radians(0.0f);
    // camera.pitch = glm::radians(-30.0f);
    // camera.pos = glm::vec3(0.0f, 0.25f, -5.0f);
    camera.yaw = glm::radians(0.0f);
    camera.pitch = glm::radians(0.0f);
    camera.pos = glm::vec3(0.0f, 1.0f, -4.0f);
    Canvas canvas(1200, 800, 3);
    std::unique_ptr scene = Scene::Create("room.json");
    TracerConfiguration config{};
    config.system.nThreads = 12u;
    config.rayTrace.ambientColor = glm::vec3(0.0f);
    config.rayTrace.nSamplesPerPixel = 16u;
    config.rayTrace.nMinBounces = 8u;
    config.rayTrace.nMaxBounces = 16u;
    // config.lens.fov = glm::radians(90.0f);
    // config.lens.fov = glm::radians(30.0f);
    LensConfiguration::fromLensParams(config.lens, 0.018f, 0.0223f, 1.8f, 4.0f);
    Tracer tracer(config);

    auto before = std::chrono::high_resolution_clock::now();

    tracer.Render(canvas, camera, *scene);

    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration(after - before);

    fmt::println("Time elapsed: {}ms", duration.count());

    canvas.SaveToPNG("out.png");

    return 0;
}