#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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

    SimpleDiffuseMaterial white(glm::vec3(0.73f));
    SimpleDiffuseMaterial red(glm::vec3(0.65f, 0.05f, 0.05f));
    SimpleDiffuseMaterial green(glm::vec3(0.12f, 0.45f, 0.15f));
    SimpleEmissiveMaterial light(glm::vec3(0.73f), glm::vec3(10.0f));

    SimpleReflectiveMaterial mirror;
    PerfectSpecularCoatedMaterial perfectSpecular(glm::vec3(0.73f), 1.5f);
    SpecularCoatedMaterial specularRed(glm::vec3(0.65f, 0.05f, 0.05f), 0.1f, 1.5f);
    SpecularCoatedMaterial specularWhite(glm::vec3(0.73f), 0.1f, 1.5f);

    std::unique_ptr floor = std::make_unique<Plane>(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    floor->SetMaterial(specularWhite);
    std::unique_ptr left = std::make_unique<Plane>(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    left->SetMaterial(red);
    std::unique_ptr right = std::make_unique<Plane>(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    right->SetMaterial(green);
    std::unique_ptr front = std::make_unique<Plane>(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    front->SetMaterial(white);
    std::unique_ptr ceiling = std::make_unique<Plane>(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    ceiling->SetMaterial(white);

    std::vector<Vertex> v;
    v.push_back(Vertex{glm::vec3(0.5f, 1.0f, 0.5f)});
    v.push_back(Vertex{glm::vec3(-0.5f, 1.0f, 0.5f)});
    v.push_back(Vertex{glm::vec3(-0.5f, 0.0f, 0.0f)});
    v.push_back(Vertex{glm::vec3(0.5f, 0.5f, 0.0f)});
    std::vector<glm::u32vec3> i;
    i.emplace_back(0, 1, 2);
    i.emplace_back(2, 3, 0);
    std::unique_ptr<Mesh> mesh = Mesh::Create(v, i);
    mesh->SetMaterial(mirror);

    std::unique_ptr sphere1 = std::make_unique<Sphere>(glm::vec3(-0.2f, 0.2f, 0.5f), 0.2f);
    sphere1->SetMaterial(specularRed);
    std::unique_ptr sphere2 = std::make_unique<Sphere>(glm::vec3(0.4f, 0.4f, 0.0f), 0.4f);
    sphere2->SetMaterial(white);

    std::unique_ptr emissive = std::make_unique<Sphere>(glm::vec3(0.0f, 2.75f, 0.0f), 1.0f);
    emissive->SetMaterial(light);

    objects.push_back(std::move(floor));
    objects.push_back(std::move(left));
    objects.push_back(std::move(right));
    objects.push_back(std::move(front));
    objects.push_back(std::move(ceiling));
    objects.push_back(std::move(mesh));
    //objects.push_back(std::move(sphere1));
    //objects.push_back(std::move(sphere2));
    objects.push_back(std::move(emissive));

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
    config.rayTrace.nRaysPerPixel = 256u;
    // config.lens.fov = glm::radians(90.0f);
    config.lens.fov = glm::radians(30.0f);
    //LensConfiguration::fromLensParams(config.lens, 0.035f, 0.022f, 1.0f, 12.0f);
    Tracer tracer(config);

    auto before = std::chrono::high_resolution_clock::now();

    tracer.Render(canvas, camera, scene);

    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration(after - before);

    std::println("Time elapsed: {}ms", duration.count());

    stbi_write_png("out.png", WIDTH, HEIGHT, 3, canvas.GetData().data(), 0);

    return 0;
}