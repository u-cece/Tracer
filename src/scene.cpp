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

void Scene::Trace(const glm::vec3& orig, const glm::vec3& dir, HitResult& hitResult) const
{
    HitResult invalid{};
    invalid.valid = false;
    if (objects.empty())
    {
        hitResult = invalid;
        return;
    }
    auto [obj, hit] = std::ranges::min(
        std::views::all(objects
                | std::views::transform([](const std::unique_ptr<Object>& obj) { return obj.get(); })
                | std::views::transform([&](const Object* obj)
                {
                    HitResult hit{};
                    std::optional<float> distance = obj->Intersect(orig, dir, hit.surfaceData);
                    if (!distance)
                        return std::make_pair(obj, invalid);
                    hit.valid = true;
                    hit.distance = distance.value();
                    hit.point = orig + dir * distance.value();
                    hit.object = obj;
                    return std::make_pair(obj, hit);
                })),
        {},
        [](std::pair<const Object*, HitResult> pair) -> float
        {
            if (!pair.second.valid)
                return std::numeric_limits<float>::infinity();
            return pair.second.distance;
        }
    );
    hitResult = hit;
}

}