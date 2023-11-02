#include <tracer/scene.h>

namespace tracer
{

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
                    std::optional<float> distance = obj->Intersect(orig, dir);
                    if (!distance)
                        return std::make_pair(obj, invalid);
                    HitResult hit{};
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