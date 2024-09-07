#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "bvh.h"
#include "camera.h"
#include "object.h"

namespace tracer
{

struct HitResult
{
    bool valid;
    float distance;
    const Object* object;

    SurfaceData surfaceData;
};

struct ObjectPtrBoxFunc
{
    AABB operator()(const BoundedObject* obj) const
    {
        return obj->GetBox();
    }
};

class Scene
{
public:
    static std::unique_ptr<Scene> Create(std::string_view path);
    auto GetObjects() const
    {
        return objects | std::views::transform([](const std::unique_ptr<Object>& ptr) -> const Object*
        {
            return ptr.get();
        });
    }
    Camera GetCamera() const { return camera; }
    glm::vec3 GetAmbientColor() const { return ambientColor; }
    void Trace(const glm::vec3& orig, const glm::vec3& dir, HitResult& hitResult) const;
private:
    Scene() {}
    void buildAccel();
    glm::vec3 ambientColor;
    Camera camera;
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<const Object*> unboundedObjects;
    BVH<const BoundedObject*, ObjectPtrBoxFunc> bvh;
};

}