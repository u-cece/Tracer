#include <tracer/scene.h>

#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include <tracer/mesh.h>

#include "json_helper.h"
#include "util.h"

namespace tracer
{

using json = nlohmann::json;

namespace
{
    glm::mat4 parseMatrixTransformationJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("value", JsonFieldType::Array);
        auto result = parser.Parse(obj);

        const json& valueObj = result.Get(0);
        if (valueObj.size() != 16u)
            throw std::runtime_error("");
        
        glm::mat4 transformation;
        for (uint32_t i = 0; i < 4; i++)
            for (uint32_t j = 0; j < 4; j++)
            {
                const json& element = valueObj[i + j * 4]; // row major
                if (!element.is_number())
                    throw std::runtime_error("");
                transformation[i][j] = element.get<float>(); // to column major
            }
        return transformation;
    }

    glm::mat4 parseTranslationTransformationJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("value", JsonFieldType::Array);
        auto result = parser.Parse(obj);

        glm::vec3 value = parseVecJson<3>(result.Get(0));
        return glm::translate(glm::mat4(1.0f), value);
    }

    glm::mat4 parseScaleTransformationJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("value", JsonFieldType::Array);
        auto result = parser.Parse(obj);

        glm::vec3 value = parseVecJson<3>(result.Get(0));
        return glm::scale(glm::mat4(1.0f), value);
    }

    glm::mat4 parseRotationTransformationJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("angle", JsonFieldType::Number);
        parser.RegisterField("axis", JsonFieldType::Array);
        auto result = parser.Parse(obj);
        
        float angle = result.Get(0);
        glm::vec3 axis = parseVecJson<3>(result.Get(1));
        return glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
    }

    std::unordered_map<std::string, std::function<glm::mat4(const json&)>> typeNameToTransformationFactory
    {
        {"matrix", parseMatrixTransformationJson},
        {"translation", parseTranslationTransformationJson},
        {"scale", parseScaleTransformationJson},
        {"rotation", parseRotationTransformationJson}
    };

    std::unique_ptr<Object> parseInlineMeshObjectJson(const json& obj, const glm::mat4& transformation)
    {
        return Mesh::Create(&obj, transformation);
    }

    std::unique_ptr<Object> parseFileMeshObjectJson(const json& obj, const glm::mat4& transformation)
    {
        JsonObjectParser parser;
        parser.RegisterField("path", JsonFieldType::String);
        auto result = parser.Parse(obj);

        std::string path = result.Get(0);
        return Mesh::Create(path, transformation);
    }

    std::unordered_map<std::string, std::function<std::unique_ptr<Object>(const json&, const glm::mat4&)>> typeNameToMeshFactory
    {
        {"inline", parseInlineMeshObjectJson},
        {"file", parseFileMeshObjectJson}
    };

    std::unique_ptr<Object> parseMeshObjectJson(const json& obj, const glm::mat4& transformation)
    {
        return parseTypedJson<std::unique_ptr<Object>>(obj, typeNameToMeshFactory, transformation);
    }

    std::unordered_map<std::string, std::function<std::unique_ptr<Object>(const json&, const glm::mat4&)>> typeNameToObjectFactory
    {
        {"mesh", parseMeshObjectJson}
    };

    std::unique_ptr<Object> parseObjectJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("transformations", JsonFieldType::Array);
        parser.RegisterField("object", JsonFieldType::Object);
        auto result = parser.Parse(obj);
        
        glm::mat4 transformation = glm::mat4(1.0f);
        for (const json& transformationObj : result.Get(0))
            transformation = transformation * parseTypedJson<glm::mat4>(transformationObj, typeNameToTransformationFactory);

        const json& objectObj = result.Get(1);
        return parseTypedJson<std::unique_ptr<Object>>(objectObj, typeNameToObjectFactory, transformation);
    }

}

std::unique_ptr<Scene> Scene::Create(std::string_view _path)
{
    std::string path(_path);
    std::string jsonStr = readTextFile(path);
    json jsonObj;
    try
    {
        jsonObj = json::parse(jsonStr);
    }
    catch(std::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    JsonObjectParser parser;
    parser.RegisterField("objects", JsonFieldType::Array);
    auto result = parser.Parse(jsonObj);

    std::unique_ptr scene = std::unique_ptr<Scene>(new Scene());
    for (const json& obj : result.Get(0))
        scene->objects.push_back(parseObjectJson(obj));
    
    scene->Build();

    return scene;
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