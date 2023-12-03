#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <string_view>
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
    static std::unique_ptr<Scene> Create(std::string_view path);
    Scene() {}
    template <std::ranges::input_range Range>
        requires (
            std::is_same_v<
                std::ranges::range_reference_t<Range>,
                std::unique_ptr<Object>&&>)
    void AddObjects(Range&& range)
    {
        std::ranges::copy(range, std::back_inserter(objects));
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