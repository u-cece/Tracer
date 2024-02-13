#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "bvh.h"
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
    template <std::ranges::input_range Range>
        requires (
            std::is_same_v<
                std::ranges::range_reference_t<Range>,
                std::unique_ptr<Object>&&>)
    static std::unique_ptr<Scene> Create(Range&& range)
    {
        std::unique_ptr scene = std::unique_ptr<Scene>(new Scene());
        std::ranges::copy(range, std::back_inserter(scene->objects));
        scene->Build();
        return scene;
    }
    auto GetObjects() const
    {
        return objects | std::views::transform([](const std::unique_ptr<Object>& ptr) -> const Object*
        {
            return ptr.get();
        });
    }
    void Trace(const glm::vec3& orig, const glm::vec3& dir, HitResult& hitResult) const;
private:
    Scene() {}
    void Build();
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<const Object*> unboundedObjects;
    BVH<const BoundedObject*, ObjectPtrBoxFunc> bvh;
};

}