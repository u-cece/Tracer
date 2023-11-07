#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>

#include <tracer/camera.h>
#include <tracer/canvas.h>
#include <tracer/mesh.h>
#include <tracer/scene.h>
#include <tracer/tracer.h>

int main()
{
    using namespace tracer;

    const uint32_t WIDTH = 512, HEIGHT = 512;

    std::vector<std::unique_ptr<Object>> objects;

    auto makeColor = [](const glm::vec3& color) { return std::make_shared<SimpleGradientTexture>(color); };

    std::shared_ptr black = makeColor(glm::vec3(0.0f));
    std::shared_ptr white = makeColor(glm::vec3(0.73f));
    std::shared_ptr red = makeColor(glm::vec3(0.65f, 0.05f, 0.05f));
    std::shared_ptr green = makeColor(glm::vec3(0.12f, 0.45f, 0.15f));
    std::shared_ptr bright = makeColor(glm::vec3(10.0f));

    SimpleDiffuseMaterial whiteDiffuse(white);
    SimpleDiffuseMaterial redDiffuse(red);
    SimpleDiffuseMaterial greenDiffuse(green);
    SimpleEmissiveMaterial emissive(white, bright);

    SimpleReflectiveMaterial mirror;
    SpecularCoatedMaterial specularRed(red, makeColor(glm::vec3(0.1f)), makeColor(glm::vec3(1.5f)));
    SpecularCoatedMaterial specularWhite(white, makeColor(glm::vec3(0.1f)), makeColor(glm::vec3(1.5f)));

    std::shared_ptr gradient = std::make_shared<SimpleGradientTexture>(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 1.0f));
    SimpleDiffuseMaterial gradientDiffuse(gradient);
    SimpleEmissiveMaterial gradientEmissive(black, gradient, 10.0f);

    std::shared_ptr image = std::make_shared<ImageTexture>("ref14.png");
    SimpleDiffuseMaterial texturedDiffuse(image);
    SimpleEmissiveMaterial texturedEmissive(black, image, 10.0f);

    std::unique_ptr floor = std::make_unique<Plane>(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    floor->SetMaterial(whiteDiffuse);
    std::unique_ptr left = std::make_unique<Plane>(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    left->SetMaterial(redDiffuse);
    std::unique_ptr right = std::make_unique<Plane>(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    right->SetMaterial(greenDiffuse);
    std::unique_ptr front = std::make_unique<Plane>(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    front->SetMaterial(whiteDiffuse);
    std::unique_ptr ceiling = std::make_unique<Plane>(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    ceiling->SetMaterial(whiteDiffuse);

    std::vector<Vertex> lightVertices;
    lightVertices.push_back(Vertex{glm::vec3(0.5f, 1.9375f, 0.5f), glm::vec2(0.0f, 1.0f)});
    lightVertices.push_back(Vertex{glm::vec3(-0.5f, 1.9375f, 0.5f), glm::vec2(1.0f, 1.0f)});
    lightVertices.push_back(Vertex{glm::vec3(-0.5f, 1.9375f, 0.0f), glm::vec2(1.0f, 0.0f)});
    lightVertices.push_back(Vertex{glm::vec3(0.5f, 1.9375f, 0.0f), glm::vec2(0.0f, 0.0f)});
    std::vector<glm::u32vec3> lightIndices;
    lightIndices.emplace_back(0, 1, 2);
    lightIndices.emplace_back(2, 3, 0);
    std::unique_ptr<Mesh> light = Mesh::Create(lightVertices, lightIndices);
    light->SetMaterial(gradientEmissive);

    std::unique_ptr sphere1 = std::make_unique<Sphere>(glm::vec3(-0.5f, 0.25f, 0.5f), 0.25f);
    sphere1->SetMaterial(gradientEmissive);
    std::unique_ptr sphere2 = std::make_unique<Sphere>(glm::vec3(-0.25f, 1.0f, 0.0f), 0.125f);
    sphere2->SetMaterial(gradientEmissive);
    std::unique_ptr sphere3 = std::make_unique<Sphere>(glm::vec3(0.25f, 0.4375f, 0.25f), 0.4375f);
    sphere3->SetMaterial(whiteDiffuse);

    objects.push_back(std::move(floor));
    objects.push_back(std::move(left));
    objects.push_back(std::move(right));
    objects.push_back(std::move(front));
    objects.push_back(std::move(ceiling));
    objects.push_back(std::move(sphere1));
    objects.push_back(std::move(sphere2));
    objects.push_back(std::move(sphere3));
    objects.push_back(std::move(light));

    Camera camera{};
    // camera.yaw = glm::radians(0.0f);
    // camera.pitch = glm::radians(-30.0f);
    // camera.pos = glm::vec3(0.0f, 0.25f, -5.0f);
    camera.yaw = glm::radians(0.0f);
    camera.pitch = glm::radians(0.0f);
    camera.pos = glm::vec3(0.0f, 1.0f, -4.0f);
    Canvas canvas(WIDTH, HEIGHT, 3);
    Scene scene;
    scene.AddObjects(objects);

    TracerConfiguration config{};
    config.system.nThreads = 6u;
    config.rayTrace.environmentColor = glm::vec3(1.0f);
    config.rayTrace.nRaysPerPixel = 65536u;
    // config.lens.fov = glm::radians(90.0f);
    config.lens.fov = glm::radians(30.0f);
    //LensConfiguration::fromLensParams(config.lens, 0.035f, 0.022f, 1.0f, 12.0f);
    Tracer tracer(config);

    auto before = std::chrono::high_resolution_clock::now();

    tracer.Render(canvas, camera, scene);

    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration(after - before);

    std::println("Time elapsed: {}ms", duration.count());

    canvas.SaveToPNG("out.png");

    return 0;
}