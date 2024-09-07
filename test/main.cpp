#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <fmt/core.h>
#include <ranges>

#include <tracer/bvh.h>
#include <tracer/camera.h>
#include <tracer/canvas.h>
#include <tracer/mesh.h>
#include <tracer/scene.h>
#include <tracer/tracer.h>

int main()
{
    using namespace tracer;

    Canvas canvas;
    std::unique_ptr scene = Scene::Create("room.json");
    TracerConfiguration config{};
    config.width = 1200u;
    config.height = 800u;
    config.nThreads = 12u;
    config.nSamplesPerPixel = 16u;
    config.nMinBounces = 8u;
    config.nMaxBounces = 16u;
    Tracer tracer(config);

    auto before = std::chrono::high_resolution_clock::now();

    tracer.Render(canvas, *scene);

    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration(after - before);

    fmt::println("Time elapsed: {}ms", duration.count());

    canvas.SaveToPNG("out.png");

    return 0;
}