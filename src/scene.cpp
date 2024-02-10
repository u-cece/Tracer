#include <tracer/scene.h>

#include <nlohmann/json.hpp>

#include "util.h"

namespace tracer
{

using json = nlohmann::json;

std::unique_ptr<Scene> Scene::Create(std::string_view _path)
{
    std::string path(_path);
    std::string jsonStr = readTextFile(path);
    json jsonObj;
    try
    {
        jsonObj = json::parse(jsonStr);
    }
    catch(const std::exception&)
    {
        throw std::runtime_error("");
    }

    if (!jsonObj.is_object())
        throw std::runtime_error("");

    return nullptr;
}

void Scene::Build()
{
    bvh.Build(objects
        | std::views::transform([](const std::unique_ptr<Object>& obj) { return obj.get(); })
        | std::views::filter([](const Object* obj) { return dynamic_cast<const BoundedObject*>(obj) != nullptr; })
        | std::views::transform([](const Object* obj) { return dynamic_cast<const BoundedObject*>(obj); }));
    std::ranges::for_each(objects
        | std::views::transform([](const std::unique_ptr<Object>& obj) { return obj.get(); })
        | std::views::filter([](const Object* obj) { return dynamic_cast<const BoundedObject*>(obj) == nullptr; }),
        [this](const Object* obj) { unboundedObjects.push_back(obj); }
    );
}

void Scene::Trace(const glm::vec3& orig, const glm::vec3& dir, HitResult& hitResult) const
{
    assert(bvh.IsBuilt());

    struct ObjectIntersectionResult
    {
        SurfaceData surfaceData{};
        const BoundedObject* obj{};
        float t{};
    };
    struct ObjectIntersectionFunc
    {
        std::optional<ObjectIntersectionResult> operator()(const BoundedObject* obj, const glm::vec3& orig, const glm::vec3& dir) const
        {
            ObjectIntersectionResult result{};
            std::optional<float> opt = obj->Intersect(orig, dir, result.surfaceData);
            if (!opt)
               return std::nullopt;
            result.obj = obj;
            result.t = opt.value();
            return result;
        }
    };
    struct ObjectDistanceFunc
    {
        float operator()(const ObjectIntersectionResult& result) const
        {
            return result.t;
        }
    };

    HitResult unboundedObjectsHit{};
    unboundedObjectsHit.valid = false;
    unboundedObjectsHit.distance = std::numeric_limits<float>::infinity();
    for (const Object* obj : unboundedObjects)
    {
        SurfaceData surfaceData{};
        std::optional<float> opt = obj->Intersect(orig, dir, surfaceData);
        if (!opt)
            continue;
        if (opt.value() < 0.0f)
            continue;
        if (opt.value() < unboundedObjectsHit.distance)
        {
            unboundedObjectsHit.valid = true;
            unboundedObjectsHit.distance = opt.value();
            unboundedObjectsHit.surfaceData = surfaceData;
            unboundedObjectsHit.object = obj;
        }
    }

    std::optional<ObjectIntersectionResult> boundedObjectsResultOpt = bvh.Intersect(orig, dir, ObjectIntersectionFunc{}, ObjectDistanceFunc{});
    HitResult boundedObjectsHit{};
    if (!boundedObjectsResultOpt)
    {
        boundedObjectsHit.valid = false;
        boundedObjectsHit.distance = std::numeric_limits<float>::infinity();
    }
    else
    {
        boundedObjectsHit.valid = true;
        boundedObjectsHit.surfaceData = boundedObjectsResultOpt.value().surfaceData;
        boundedObjectsHit.object = boundedObjectsResultOpt.value().obj;
        boundedObjectsHit.distance = boundedObjectsResultOpt.value().t;
    }
    
    if (!unboundedObjectsHit.valid && !boundedObjectsHit.valid)
    {
        hitResult.valid = false;
        return;
    }
    
    hitResult =
        unboundedObjectsHit.distance < boundedObjectsHit.distance ?
        unboundedObjectsHit : boundedObjectsHit;
}

}