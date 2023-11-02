#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include <glm/glm.hpp>

#include "material.h"

namespace tracer
{

// enum class AttributeType
// {
    
// };

// template <AttributeType>
// struct AttributeTypeOf {};

// template <AttributeType type>
// auto ConvertToAttribute(const void* attrib)
// {
//     return reinterpret_cast<const AttributeTypeOf<type>::type*>(attrib);
// }

struct SurfaceData
{
    glm::vec3 normal;
    std::optional<glm::vec2> texCoords;
    const Material* material;
};

class Object
{
public:
    Object() = default;
    // template <AttributeType attribType, typename... Args>
    // void EmplaceAttribute(Args&& ...args)
    // {
    //     using T = AttributeTypeOf<attribType>::type;
    //     attributes.insert({attribType,
    //         void_unique_ptr(new T(std::forward<Args>(args)...),
    //             [](const void* data)
    //             {
    //                 const T* p = reinterpret_cast<const T*>(data);
    //                 delete p;
    //             })});
    // }
    // template <AttributeType attribType>
    // auto GetAttribute() const -> const typename AttributeTypeOf<attribType>::type*
    // {
    //     if (attributes.contains(attribType))
    //         return ConvertToAttribute<attribType>(attributes.at(attribType).get());
    //     return nullptr;
    // }
    virtual std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir) const = 0;
    virtual void GetSurfaceData(const glm::vec3& point, SurfaceData& data) const = 0;
    virtual ~Object() {}
private:
    using void_unique_ptr = std::unique_ptr<void, void(*)(const void*)>;
    //std::unordered_map<AttributeType, void_unique_ptr> attributes;
};

class SimpleMaterialObject : public Object
{
public:
    SimpleMaterialObject() = default;
    virtual void GetSurfaceData(const glm::vec3& point, SurfaceData& data) const
    {
        data.material = material.get();
    }
    template <typename T>
        requires std::is_base_of_v<Material, std::remove_reference_t<T>>
    void SetMaterial(T&& mat)
    {
        material = std::make_unique<std::remove_reference_t<T>>(std::forward<T>(mat));
    }
private:
    std::unique_ptr<Material> material;
};

class Sphere : public SimpleMaterialObject
{
public:
    Sphere(const glm::vec3& origin, float radius)
        : origin(origin), radius{radius}, radiusSquared{radius * radius}
    {}
    std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir) const override;
    void GetSurfaceData(const glm::vec3& point, SurfaceData& data) const override;
private:
    glm::vec3 origin;
    float radius;
    float radiusSquared;
};

class Plane : public SimpleMaterialObject
{
public:
    Plane(const glm::vec3& origin, const glm::vec3& normal)
        : origin(origin), normal(normal)
    {}
    std::optional<float> Intersect(const glm::vec3& orig, const glm::vec3& dir) const override;
    void GetSurfaceData(const glm::vec3& point, SurfaceData& data) const override;
private:
    glm::vec3 origin, normal;
};
}