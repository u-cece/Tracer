#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>

#include <glm/glm.hpp>

#include "object.h"

namespace tracer
{

struct HitResult
{
    bool valid;
    glm::vec3 point;
    float distance;
    const Object* object;

    SurfaceData surfaceData;
};

class Scene
{
public:
    Scene() {}
    template <std::ranges::input_range Range>
        requires (
            std::is_same_v<
                std::ranges::range_value_t<Range>,
                std::unique_ptr<Object>>)
    void AddObjects(Range&& range)
    {
        std::ranges::for_each(range, [this](std::unique_ptr<Object>& obj)
        {
            objects.push_back(std::move(obj));
        });
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
    std::vector<std::unique_ptr<Object>> objects;
};
    
}