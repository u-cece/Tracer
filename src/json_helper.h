#pragma once

#include <ranges>
#include <string_view>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace tracer
{

template <uint32_t vecLength>
auto parseVecJson(const nlohmann::json& obj)
{
    if (!obj.is_array())
        throw std::runtime_error("");
    
    for (const nlohmann::json& v : obj)
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

template <typename T, typename Container, typename... Args>
T parseTypedJson(const nlohmann::json& obj, Container typeNameToFactory, Args&&... args)
{
    if (!obj.is_object())
        throw std::runtime_error("");
    
    auto typeIt = obj.find("type");
    if (typeIt == obj.end() || !typeIt->is_string())
        throw std::runtime_error("");
    
    auto contentIt = obj.find("content");
    if (contentIt == obj.end() || !contentIt->is_object())
        throw std::runtime_error("");

    std::string type = typeIt->get<std::string>();
    if (!typeNameToFactory.contains(type))
        throw std::runtime_error("");

    return typeNameToFactory.at(type)(*contentIt, std::forward<Args>(args)...);
}

enum class JsonFieldType
{
    Object, Array, Float, Integer, Number, String, Boolean
};

class JsonObjectParseResult
{
    friend class JsonObjectParser;
public:
    template<typename T>
    auto Get(uint64_t index) const
    {
        return fields.at(index)->get<T>();
    }
    const nlohmann::json& Get(uint64_t index) const
    {
        return *fields.at(index);
    }
private:
    JsonObjectParseResult() {}
    std::vector<nlohmann::json::const_iterator> fields;
};

class JsonObjectParser
{
    struct FieldInfo
    {
        std::string name;
        JsonFieldType type;
        std::optional<nlohmann::json> defaultValue;
    };
public:
    JsonObjectParser() {}
    void RegisterField(std::string_view name, JsonFieldType type)
    {
        FieldInfo info{};
        info.name = name;
        info.type = type;
        fieldInfos.emplace_back(std::move(info));
    }
    template <typename T = void>
    void RegisterField(std::string_view name, JsonFieldType type, T&& defaultValue)
    {
        FieldInfo info{};
        info.name = name;
        info.type = type;
        info.defaultValue.emplace(std::forward<T>(defaultValue));
        fieldInfos.emplace_back(std::move(info));
    }
    JsonObjectParseResult Parse(const nlohmann::json& obj) const
    {
        using nlohmann::json;

        if (!obj.is_object())
            throw std::runtime_error("");

        JsonObjectParseResult result;
        for (const FieldInfo& fieldInfo : fieldInfos)
        {
            json::const_iterator fieldIt = obj.find(fieldInfo.name);
            if (fieldIt == obj.end())
            {
                if (!fieldInfo.defaultValue)
                    throw std::runtime_error("");
                fieldIt = json::const_iterator(&fieldInfo.defaultValue.value());
            }
            verifyField(fieldIt, fieldInfo.type);
            result.fields.push_back(fieldIt);
        }

        for (auto& [key, value] : obj.items())
        {
            if (std::ranges::find(ignoredFields, key) != ignoredFields.end())
                continue;
            json::const_iterator it(&value);
            if (std::ranges::find_if(
                result.fields,
                [&](json::const_iterator it)
                { return &value == &*it; }) == result.fields.end())
                throw std::runtime_error("");
        }
        return result;
    }
protected:
    JsonObjectParser(std::vector<std::string>&& ignored) : ignoredFields(std::move(ignored)) {}
private:
    static void verifyField(nlohmann::json::const_iterator it, JsonFieldType type)
    {
        switch (type)
        {
            case JsonFieldType::Object:
            {
                if (!it->is_object())
                    throw std::runtime_error("");
                break;
            }
            case JsonFieldType::Float:
            {
                if (!it->is_number_float())
                    throw std::runtime_error("");
                break;
            }
            case JsonFieldType::Integer:
            {
                if (!it->is_number_integer())
                    throw std::runtime_error("");
                break;
            }
            case JsonFieldType::Number:
            {
                if (!it->is_number())
                    throw std::runtime_error("");
                break;
            }
            case JsonFieldType::Boolean:
            {
                if (!it->is_boolean())
                    throw std::runtime_error("");
                break;
            }
        }
    }
    std::vector<FieldInfo> fieldInfos;
    std::vector<std::string> ignoredFields;
};

// class TypedObjectParser : public JsonObjectParser
// {
// public:
//     TypedObjectParser() : JsonObjectParser({"type"}) {}
// };

}