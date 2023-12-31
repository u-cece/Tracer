#include <tracer/mesh.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <optional>
#include <print>
#include <ranges>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "util.h"

namespace tracer
{

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{

    template <std::ranges::input_range Range, typename It>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>, std::string>
    void findRequiredFields(const json& obj, Range&& range, It backInserter)
    {
        auto elements = range
            | std::views::transform([&](const auto& str)
            {
                return obj.find(str);
            })
            | std::ranges::to<std::vector>();
        if (std::ranges::find(elements, obj.end()) != elements.end()) // if any of the elements is missing, throw
            throw std::runtime_error("");
        for (auto it = obj.begin(); it != obj.end(); it++) // if any other element exists, throw
            if (std::ranges::find(elements, it) == elements.end())
                throw std::runtime_error("");
        
        for (json::const_iterator it : elements)
            *backInserter = it;
    }

    template <typename T, typename Container, typename... Args>
    T parseTypedJson(const json& obj, Container typeNameToFactory, Args... args)
    {
        if (!obj.is_object())
            throw std::runtime_error("");
        
        auto typeIt = obj.find("type");
        if (typeIt == obj.end() || !typeIt->is_string())
            throw std::runtime_error("");
        
        std::string type = typeIt->get<std::string>();
        if (!typeNameToFactory.contains(type))
            throw std::runtime_error("");

        return typeNameToFactory.at(type)(obj, std::forward<Args>(args)...);
    }

    template <uint32_t vecLength>
    auto parseVecJson(const json& obj)
    {
        if (!obj.is_array())
            throw std::runtime_error("");
        
        for (const json& v : obj)
            if (!v.is_number())
                throw std::runtime_error("");

        auto container = obj.get<std::vector<float>>();
        if (container.size() != vecLength)
            throw std::runtime_error("");
        
        glm::vec<vecLength, float, glm::defaultp> vec;
        for (auto [i, v] : container | std::views::enumerate)
            vec[static_cast<uint32_t>(i)] = v;
        
        return vec;
    }

    std::shared_ptr<Texture> parseGradientTextureJson(const json& obj)
    {
        std::vector<json::const_iterator> elements;
        auto required = {"type", "top-left", "top-right", "bottom-right", "bottom-left"};
        findRequiredFields(obj, required, std::back_inserter(elements));

        if (std::find_if_not(elements.begin() + 1, elements.end(), [](auto it) { return it->is_array(); }) != elements.end()) // if any of them isn't an array
            throw std::runtime_error("");
        
        glm::vec3 topL, topR, botR, botL;
        topL = parseVecJson<3>(*elements.at(1));
        topR = parseVecJson<3>(*elements.at(2));
        botR = parseVecJson<3>(*elements.at(3));
        botL = parseVecJson<3>(*elements.at(4));

        return std::make_shared<SimpleGradientTexture>(topL, topR, botR, botL);
    }

    std::shared_ptr<Texture> parsePlainColorTextureJson(const json& obj)
    {
        std::vector<json::const_iterator> elements;
        auto required = {"type", "color"};
        findRequiredFields(obj, required, std::back_inserter(elements));

        if (!elements.at(1)->is_array())
            throw std::runtime_error("");
        
        glm::vec3 color = parseVecJson<3>(*elements.at(1));

        return std::make_shared<SimpleGradientTexture>(color);
    }

    std::shared_ptr<Texture> parseImageTextureJson(const json& obj)
    {
        std::vector<json::const_iterator> elements;
        auto required = {"type", "path"};
        findRequiredFields(obj, required, std::back_inserter(elements));

        if (!elements.at(1)->is_string())
            throw std::runtime_error("");

        std::string path = elements.at(1)->get<std::string>();

        if (!fs::is_regular_file(path))
            throw std::runtime_error("");

        return std::make_shared<ImageTexture>(path);
    }

    std::unordered_map<std::string, std::function<std::shared_ptr<Texture>(const json&)>> typeNameToTextureFactory
    {{"gradient", parseGradientTextureJson}, {"plain-color", parsePlainColorTextureJson}, {"image", parseImageTextureJson}};

    Vertex parseVertexJson(const json& obj)
    {
        if (!obj.is_object())
            throw std::runtime_error("");

        auto posIt = obj.find("pos");
        auto texIt = obj.find("tex");

        if (posIt == obj.end() || !posIt->is_array())
            throw std::runtime_error("");

        for (auto it = obj.begin(); it != obj.end(); it++)
            if (it != posIt && it != texIt)
                throw std::runtime_error("");

        Vertex vertex{};

        vertex.pos = parseVecJson<3>(*posIt);

        if (texIt != obj.end())
            vertex.texCoords = parseVecJson<2>(*texIt);
        else
            vertex.texCoords = glm::vec2(0.0f);

        return vertex;
    }

    void parseTriadJson(const json& obj, glm::u32vec3& indices, uint32_t& materialIndex)
    {
        if (!obj.is_object())
            throw std::runtime_error("");

        std::vector<json::const_iterator> elements;
        auto required = {"material-index", "0", "1", "2"};
        findRequiredFields(obj, required, std::back_inserter(elements));

        for (auto it : elements) // if any of the elements is not an integer, throw
            if (!it->is_number_integer())
                throw std::runtime_error("");

        materialIndex = elements.at(0)->get<uint32_t>();
        indices = glm::u32vec3(elements.at(1)->get<uint32_t>(), elements.at(2)->get<uint32_t>(), elements.at(3)->get<uint32_t>());
    }

    const std::shared_ptr<Texture>& parseTextureIndexJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        if (!obj.is_number_integer())
            throw std::runtime_error("");
        
        uint64_t index = obj.get<uint64_t>();
        if (index >= textures.size())
            throw std::runtime_error("");
        return textures.at(index);
    }

    std::unique_ptr<Material> parseSimpleDiffuseMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        auto required = {"type", "diffuse-texture"};
        std::vector<json::const_iterator> elements;
        findRequiredFields(obj, required, std::back_inserter(elements));

        const auto& diffuse = parseTextureIndexJson(*elements.at(1), textures);

        return std::make_unique<SimpleDiffuseMaterial>(diffuse);
    }

    std::unique_ptr<Material> parseSpecularCoatedMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        auto required = {"type", "diffuse-texture", "roughness-texture", "ior"};
        std::vector<json::const_iterator> elements;
        findRequiredFields(obj, required, std::back_inserter(elements));

        const auto& diffuse = parseTextureIndexJson(*elements.at(1), textures);
        const auto& roughness = parseTextureIndexJson(*elements.at(2), textures);
        float ior = elements.at(3)->get<float>();

        return std::make_unique<SpecularCoatedMaterial>(diffuse, roughness, ior);
    }

    std::unique_ptr<Material> parseSimpleEmissiveMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        auto required = {"type", "emissive-texture", "multiplier"};
        std::vector<json::const_iterator> elements;
        findRequiredFields(obj, required, std::back_inserter(elements));

        if (!elements.at(1)->is_number_integer())
            throw std::runtime_error("");
        if (!elements.at(2)->is_number())
            throw std::runtime_error("");
        
        const auto& emissive = parseTextureIndexJson(*elements.at(1), textures);
        float multiplier = elements.at(2)->get<float>();

        return std::make_unique<SimpleEmissiveMaterial>(emissive, multiplier);
    }

    std::unique_ptr<Material> parsePerfectRefractiveMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        return nullptr;
    }

    std::unordered_map<std::string, std::function<std::unique_ptr<Material>(const json&, const std::vector<std::shared_ptr<Texture>>&)>> typeNameToMaterialFactory
    {{"SimpleDiffuse", parseSimpleDiffuseMaterialJson}, {"SpecularCoated", parseSpecularCoatedMaterialJson}, {"SimpleEmissive", parseSimpleEmissiveMaterialJson}, {"PerfectRefractive", parsePerfectRefractiveMaterialJson}};

    std::unordered_map<
        std::string, std::function<
            void(
                const json&,
                const std::vector<std::shared_ptr<Texture>>&,
                const std::vector<Vertex>&, std::back_insert_iterator<std::vector<std::unique_ptr<Material>>>,
                std::back_insert_iterator<std::vector<Triad>>,
                std::optional<LightInfo>&
            )>> typeNameToPrimitiveFactory
    {};

    void parsePrimitiveJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures,
        const std::vector<Vertex>& vertices,
        std::back_insert_iterator<std::vector<std::unique_ptr<Material>>> materialsInserter,
        std::back_insert_iterator<std::vector<Triad>> triadsInserter,
        std::optional<LightInfo>& lightInfo)
    {
        if (!obj.is_object())
            throw std::runtime_error("");

        auto typeIt = obj.find("type");
        if (typeIt == obj.end() || !typeIt->is_string())
            throw std::runtime_error("");
        std::string type = typeIt->get<std::string>();

        std::vector<json::const_iterator> elements;
        auto requiredTranslucent = {"type", "surface-materials", "indices", "ior"};
        auto requiredSolid = {"type", "surface-materials", "indices"};
        findRequiredFields(obj, type == "translucent" ? requiredTranslucent : requiredSolid, std::back_inserter(elements));

        if (!elements.at(1)->is_array())
            throw std::runtime_error("");
        if (!elements.at(2)->is_array())
            throw std::runtime_error("");
        
        auto iorIt = obj.find("ior");
        float ior = 1.5f;
        if (type == "translucent" && iorIt != obj.end())
        {
            if (!iorIt->is_number())
                throw std::runtime_error("");
            ior = iorIt->get<float>();
        }

        std::vector<std::pair<glm::u32vec3, uint32_t>> indicesToMaterialIndex;
        for (const json& obj : *elements.at(2))
        {
            std::pair<glm::u32vec3, uint32_t> pair;
            parseTriadJson(obj, pair.first, pair.second);
            indicesToMaterialIndex.push_back(pair);
        }

        std::vector<std::unique_ptr<Material>> materials;
        for (const json& obj : *elements.at(1))
        {
            materials.push_back(parseTypedJson<std::unique_ptr<Material>>(obj, typeNameToMaterialFactory, textures));
        }

        std::vector<std::array<glm::vec3, 3>> emissiveTriads;

        for (const auto& [indices, materialIndex] : indicesToMaterialIndex)
        {
            Triad triad{};
            for (uint32_t i = 0; i < 3; i++)
            {
                uint32_t index = indices[i];
                if (index >= vertices.size())
                    throw std::runtime_error("");
                triad.vertices.at(i) = vertices.at(index);
            }
            if (materialIndex >= materials.size())
                throw std::runtime_error("");
            triad.material = materials.at(materialIndex).get();
            *triadsInserter = triad;

            if (triad.material->IsEmissive())
            {
                std::array<glm::vec3, 3> triadPos;
                for (uint32_t i = 0; i < 3; i++)
                    triadPos.at(i) = triad.vertices.at(i).pos;
                emissiveTriads.push_back(triadPos);
            }
        }

        if (!emissiveTriads.empty())
        {
            LightInfo info{};
            info.nTriads = static_cast<uint32_t>(emissiveTriads.size());
            info.triads = std::make_unique<std::array<glm::vec3, 3>[]>(emissiveTriads.size());
            for (uint32_t i = 0; i < info.nTriads; i++)
            {
                info.triads[i] = emissiveTriads.at(i);
            }
            lightInfo.emplace(std::move(info));
        }

        std::ranges::copy(materials | std::views::as_rvalue, materialsInserter);
    }

}

std::unique_ptr<Mesh> Mesh::Create(std::string_view _path, const glm::mat4& transformation)
{
    std::string path(_path);
    std::string jsonStr = readTextFile(path);
    json jsonObj;
    try
    {
        jsonObj = json::parse(jsonStr);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("");
    }

    if (!jsonObj.is_object())
        throw std::runtime_error("");

    std::vector<json::const_iterator> elements;
    auto required = {"textures", "primitives", "vertices", "cull-mode"};
    findRequiredFields(jsonObj, required, std::back_inserter(elements));

    if (std::find_if_not(elements.begin(), elements.begin() + 3, [](json::const_iterator element)
        { return element->is_array(); }) != elements.begin() + 3)
        throw std::runtime_error("");
    if (!elements.at(3)->is_string())
        throw std::runtime_error("");

    std::string cullModeStr = elements.at(3)->get<std::string>();
    CullMode cullMode;
    if (cullModeStr == "none")
        cullMode = CullMode::None;
    else if (cullModeStr == "back")
        cullMode = CullMode::Back;
    else if (cullModeStr == "front")
        cullMode = CullMode::Front;
    else
        throw std::runtime_error("");

    std::vector<std::shared_ptr<Texture>> textures;
    for (const json& obj : *elements.at(0))
        textures.push_back(parseTypedJson<std::shared_ptr<Texture>>(obj, typeNameToTextureFactory));

    std::vector<Vertex> vertices;
    for (const json& obj : *elements.at(2))
        vertices.push_back(parseVertexJson(obj));

    std::vector<std::unique_ptr<Material>> materials;
    std::vector<Triad> triads;
    std::vector<LightInfo> lightInfos;
    for (const json& obj : *elements.at(1))
    {
        std::optional<LightInfo> lightInfo;
        parsePrimitiveJson(obj, textures, vertices, std::back_inserter(materials), std::back_inserter(triads), lightInfo);
        if (lightInfo)
            lightInfos.push_back(std::move(lightInfo.value()));
    }

    std::unique_ptr<Mesh> mesh(new Mesh());
    mesh->cullMode = cullMode;
    mesh->materialHolder = std::move(materials);
    mesh->triads = std::move(triads);
    mesh->lightInfos = std::move(lightInfos);

    mesh->Transform(transformation);

    return mesh;
}

std::optional<float> Mesh::Intersect(const glm::vec3& orig, const glm::vec3& dir, SurfaceData& surfaceData) const
{
    using namespace glm;
    
    assert(accelStruct.IsBuilt());

    struct TriadIntersectionResult
    {
        SurfaceData surfaceData{};
        float t{};
    };
    struct TriadIntersectionFunc
    {
        CullMode cullMode{};
        std::optional<TriadIntersectionResult> operator()(const Triad& triad, const glm::vec3& orig, const glm::vec3& dir) const
        {
            Vertex v0 = triad.vertices.at(0);
            Vertex v1 = triad.vertices.at(1);
            Vertex v2 = triad.vertices.at(2);

            vec3 p0 = v0.pos;
            vec3 p1 = v1.pos;
            vec3 p2 = v2.pos;

            vec2 t0 = v0.texCoords;
            vec2 t1 = v1.texCoords;
            vec2 t2 = v2.texCoords;

            vec2 coords;
            std::optional<float> t;
            vec3 normal;

            if (cullMode == CullMode::None)
            {
                t = intersectTriangleMT(orig, dir, p0, p1, p2, coords);
                bool clockwise;
                if (!(clockwise = t.has_value()))
                    t = intersectTriangleCounterClockwiseMT(orig, dir, p0, p1, p2, coords);
                if (!t)
                    return std::nullopt;
                normal = clockwise ?
                cross(p2 - p0, p1 - p0) :
                cross(p1 - p0, p2 - p0);
            }
            else if (cullMode == CullMode::Back)
            {
                t = intersectTriangleMT(orig, dir, p0, p1, p2, coords);
                if (!t)
                    return std::nullopt;
                normal = cross(p2 - p0, p1 - p0);
            }
            else if (cullMode == CullMode::Front)
            {
                t = intersectTriangleCounterClockwiseMT(orig, dir, p0, p1, p2, coords);
                if (!t)
                    return std::nullopt;
                normal = cross(p1 - p0, p2 - p0);
            }

            SurfaceData data{};
            normal = normalize(normal);
            data.normal = normal;
            vec2 ab = t1 - t0;
            vec2 ac = t2 - t0;
            data.texCoords = t0 + ab * coords.x + ac * coords.y;
            data.material = triad.material;
            return TriadIntersectionResult{data, t.value()};
        }
    };
    struct TriadDistanceFunc
    {
        float operator()(const TriadIntersectionResult& result) const
        {
            return result.t;
        }
    };

    std::optional<TriadIntersectionResult> result =
        accelStruct.Intersect(
            orig, dir,
            TriadIntersectionFunc{cullMode},
            TriadDistanceFunc{}
        );
    if (!result)
        return std::nullopt;
    surfaceData = result.value().surfaceData;
    return result.value().t;
}

void Mesh::GetEmissionProfiles(std::back_insert_iterator<std::vector<std::unique_ptr<EmissionProfile>>> profilesInserter) const
{
    struct TriadsEmissionProfile : public EmissionProfile
    {
        const LightInfo& lightInfo;
        CullMode cullMode;
        TriadsEmissionProfile(const LightInfo& lightInfo, CullMode cullMode) : lightInfo(lightInfo), cullMode(cullMode)
        {
        }
        std::optional<EmissionSample> Sample(RNG& rng, const glm::vec3& orig, const glm::vec3& pNorm) const override
        {
            using namespace glm;

            bool doubleFaced = cullMode == CullMode::None;
            
            std::vector<vec3> samples;
            std::vector<vec3> normals;
            for (uint32_t i = 0; i < lightInfo.nTriads; i++)
            {
                vec3 normal;
                vec3 ab = lightInfo.triads[i].at(1) - lightInfo.triads[i].at(0);
                vec3 ac = lightInfo.triads[i].at(2) - lightInfo.triads[i].at(0);
                if (cullMode == CullMode::Front)
                    normal = cross(ab, ac);
                else
                    normal = cross(ac, ab);
                normal = normalize(normal);

                vec3 sample;
                float pdf;
                sampleTriangleUniform(rng, lightInfo.triads[i].at(0), lightInfo.triads[i].at(1), lightInfo.triads[i].at(2), sample, pdf);

                vec3 dir = normalize(sample - orig);

                samples.push_back(dir);
                normals.push_back(normal);
            }

            if (samples.empty())
                return std::nullopt;

            int selectedIndex = rng.Uniform(0, static_cast<uint32_t>(samples.size()) - 1);
            vec3 sample = samples.at(selectedIndex);
            vec3 normal = normals.at(selectedIndex);

            if (!doubleFaced && dot(-sample, normal) <= 0.0f)
                return std::nullopt;
            if (dot(sample, pNorm) <= 0.0f)
                return std::nullopt;

            EmissionSample emissionSample{};
            emissionSample.sample = sample;

            return emissionSample;
        }
        virtual float GetPdf(const glm::vec3& orig, const glm::vec3& dir) const override
        {
            float accumPdf = 0.0f;
            for (uint32_t i = 0; i < lightInfo.nTriads; i++)
            {
                auto points = lightInfo.triads[i];
                glm::vec3 p0 = points.at(0);
                glm::vec3 p1 = points.at(1);
                glm::vec3 p2 = points.at(2);
                glm::vec2 coords;
                std::optional<float> t;
                glm::vec3 normal;
                if (cullMode == CullMode::None)
                {
                    t = intersectTriangleMT(orig, dir, p0, p1, p2, coords);
                    bool clockwise;
                    if (!(clockwise = t.has_value()))
                        t = intersectTriangleCounterClockwiseMT(orig, dir, p0, p1, p2, coords);
                    if (!t)
                        continue;
                    normal = clockwise ?
                    cross(p2 - p0, p1 - p0) :
                    cross(p1 - p0, p2 - p0);
                }
                else if (cullMode == CullMode::Back)
                {
                    t = intersectTriangleMT(orig, dir, p0, p1, p2, coords);
                    if (!t)
                        continue;
                    normal = cross(p2 - p0, p1 - p0);
                }
                else if (cullMode == CullMode::Front)
                {
                    t = intersectTriangleCounterClockwiseMT(orig, dir, p0, p1, p2, coords);
                    if (!t)
                        continue;
                    normal = cross(p1 - p0, p2 - p0);
                }
                normal = normalize(normal);
                float distance = t.value();
                float area = 0.5f * length(cross(p1 - p0, p2 - p0));
                accumPdf += distance * distance / (abs(dot(-dir, normal)) * area);
            }

            return accumPdf / static_cast<float>(lightInfo.nTriads);
        }
    };
    for (const LightInfo& lightInfo : lightInfos)
        *profilesInserter = std::make_unique<TriadsEmissionProfile>(lightInfo, cullMode);
}

}