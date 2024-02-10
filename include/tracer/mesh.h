#pragma once

#include <array>
#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "bvh.h"
#include "material.h"
#include "object.h"

namespace tracer
{

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texCoords;
};

struct Triad
{
    std::array<Vertex, 3> vertices;
    const Material* material;
};

struct TriadBoxFunc
{
    AABB operator()(const Triad& triad) const
    {
        AABB box(triad.vertices.at(0).pos, triad.vertices.at(1).pos);
        box.Grow(triad.vertices.at(2).pos);
        return box;
    }
};

struct LightInfo
{
    std::unique_ptr<std::array<glm::vec3, 3>[]> triads;
    uint32_t nTriads;
};

enum class CullMode // front = clockwise, back = counter-clockwise
{
    None, Front, Back
};

class Mesh : public BoundedObject
{
public:
    static std::unique_ptr<Mesh> Create(std::string_view path, const glm::mat4& transformation = {});
    AABB GetBox() const override
    {
        return accelStruct.GetBox();
    }
    void Transform(const glm::mat4& matrix)
    {
        for (Triad& triad : triads)
            for (Vertex& vertex : triad.vertices)
            {
                glm::vec4 v = matrix * glm::vec4(vertex.pos, 1.0f);
                vertex.pos = glm::vec3(v);
            }
        accelStruct.Build(triads);
        for (LightInfo& lightInfo : lightInfos)
            for (uint32_t i = 0; i < lightInfo.nTriads; i++)
                for (uint32_t j = 0; j < 3; j++)
                {
                    glm::vec4 v = matrix * glm::vec4(lightInfo.triads[i][j], 1.0f);
                    lightInfo.triads[i][j] = glm::vec3(v);
                }
    }
    virtual std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const override;
    virtual void GetEmissionProfiles(std::back_insert_iterator<std::vector<std::unique_ptr<EmissionProfile>>> profilesInserter) const override;
private:
    enum class PrimitiveType
    {
        Solid, Translucent
    };

    Mesh() {}

    std::vector<std::unique_ptr<Material>> materialHolder;
    std::vector<Triad> triads;
    BVH<Triad, TriadBoxFunc> accelStruct;
    CullMode cullMode;

    std::vector<LightInfo> lightInfos;
};

}