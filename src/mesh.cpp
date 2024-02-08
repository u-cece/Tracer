#include <tracer/mesh.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <optional>
#include <print>
#include <ranges>
#include <unordered_map>

#include "json_helper.h"
#include "util.h"

namespace tracer
{

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{

    std::shared_ptr<Texture> parseGradientTextureJson(const json& obj)
    {
        TypedObjectParser parser;
        parser.RegisterField("top-left", JsonFieldType::Array);
        parser.RegisterField("top-right", JsonFieldType::Array);
        parser.RegisterField("bottom-right", JsonFieldType::Array);
        parser.RegisterField("bottom-left", JsonFieldType::Array);
        auto result = parser.Parse(obj);
        
        glm::vec3 topL, topR, botR, botL;
        topL = parseVecJson<3>(result.Get(0));
        topR = parseVecJson<3>(result.Get(1));
        botR = parseVecJson<3>(result.Get(2));
        botL = parseVecJson<3>(result.Get(3));

        return std::make_shared<SimpleGradientTexture>(topL, topR, botR, botL);
    }

    std::shared_ptr<Texture> parsePlainColorTextureJson(const json& obj)
    {
        TypedObjectParser parser;
        parser.RegisterField("color", JsonFieldType::String);
        auto result = parser.Parse(obj);
        
        glm::vec3 color = parseVecJson<3>(result.Get(0));

        return std::make_shared<SimpleGradientTexture>(color);
    }

    std::shared_ptr<Texture> parseImageTextureJson(const json& obj)
    {
        TypedObjectParser parser;
        parser.RegisterField("path", JsonFieldType::String);
        auto result = parser.Parse(obj);

        std::string path = result.Get<std::string>(0);

        if (!fs::is_regular_file(path))
            throw std::runtime_error("");

        return std::make_shared<ImageTexture>(path);
    }

    std::unordered_map<std::string, std::function<std::shared_ptr<Texture>(const json&)>> typeNameToTextureFactory
    {{"gradient", parseGradientTextureJson}, {"plain-color", parsePlainColorTextureJson}, {"image", parseImageTextureJson}};

    Vertex parseVertexJson(const json& obj)
    {
        JsonObjectParser parser;
        parser.RegisterField("pos", JsonFieldType::Array);
        parser.RegisterField("tex", JsonFieldType::Array);
        auto result = parser.Parse(obj);

        Vertex vertex{};
        vertex.pos = parseVecJson<3>(result.Get(0));
        vertex.texCoords = parseVecJson<2>(result.Get(1));

        return vertex;
    }

    void parseTriadJson(const json& obj, glm::u32vec3& indices, uint32_t& materialIndex)
    {
        JsonObjectParser parser;
        parser.RegisterField("material-index", JsonFieldType::Integer);
        parser.RegisterField("0", JsonFieldType::Integer);
        parser.RegisterField("1", JsonFieldType::Integer);
        parser.RegisterField("2", JsonFieldType::Integer);
        auto result = parser.Parse(obj);

        materialIndex = result.Get<uint32_t>(0);
        indices = glm::u32vec3(result.Get<uint32_t>(1), result.Get<uint32_t>(2), result.Get<uint32_t>(3));
    }

    std::unique_ptr<Material> parseSimpleDiffuseMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        TypedObjectParser parser;
        parser.RegisterField("diffuse-texture", JsonFieldType::Integer);
        auto result = parser.Parse(obj);

        const auto& diffuse = textures.at(result.Get<uint64_t>(0));

        return std::make_unique<SimpleDiffuseMaterial>(diffuse);
    }

    std::unique_ptr<Material> parseSpecularCoatedMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        TypedObjectParser parser;
        parser.RegisterField("diffuse-texture", JsonFieldType::Integer);
        parser.RegisterField("roughness-texture", JsonFieldType::Integer);
        parser.RegisterField("ior", JsonFieldType::Number);
        auto result = parser.Parse(obj);

        const auto& diffuse = textures.at(result.Get<uint64_t>(0));
        const auto& roughness = textures.at(result.Get<uint64_t>(1));
        float ior = result.Get<float>(2);

        return std::make_unique<SpecularCoatedMaterial>(diffuse, roughness, ior);
    }

    std::unique_ptr<Material> parseSimpleEmissiveMaterialJson(const json& obj, const std::vector<std::shared_ptr<Texture>>& textures)
    {
        TypedObjectParser parser;
        parser.RegisterField("emissive-texture", JsonFieldType::Integer);
        parser.RegisterField("multiplier", JsonFieldType::Number);
        auto result = parser.Parse(obj);
        
        const auto& emissive = textures.at(result.Get<uint64_t>(0));
        float multiplier = result.Get<float>(1);

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
        JsonObjectParser parser;
        parser.RegisterField("type", JsonFieldType::String);
        parser.RegisterField("surface-materials", JsonFieldType::Array);
        parser.RegisterField("indices", JsonFieldType::Array);
        auto result = parser.Parse(obj);

        std::string type = result.Get<std::string>(0);
        if (type != "solid")
            throw std::runtime_error("");

        std::vector<std::pair<glm::u32vec3, uint32_t>> indicesToMaterialIndex;
        for (const json& obj : result.Get(2))
        {
            std::pair<glm::u32vec3, uint32_t> pair;
            parseTriadJson(obj, pair.first, pair.second);
            indicesToMaterialIndex.push_back(pair);
        }

        std::vector<std::unique_ptr<Material>> materials;
        for (const json& obj : result.Get(1))
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
    catch(const std::exception&)
    {
        throw std::runtime_error("");
    }

    JsonObjectParser parser;
    parser.RegisterField("textures", JsonFieldType::Array);
    parser.RegisterField("primitives", JsonFieldType::Array);
    parser.RegisterField("vertices", JsonFieldType::Array);
    parser.RegisterField("cull-mode", JsonFieldType::String);
    auto result = parser.Parse(jsonObj);

    std::string cullModeStr = result.Get<std::string>(3);
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
    for (const json& obj : result.Get(0))
        textures.push_back(parseTypedJson<std::shared_ptr<Texture>>(obj, typeNameToTextureFactory));

    std::vector<Vertex> vertices;
    for (const json& obj : result.Get(2))
        vertices.push_back(parseVertexJson(obj));

    std::vector<std::unique_ptr<Material>> materials;
    std::vector<Triad> triads;
    std::vector<LightInfo> lightInfos;
    for (const json& obj : result.Get(1))
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