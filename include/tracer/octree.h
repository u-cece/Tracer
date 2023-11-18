#pragma once

#include <array>
#include <memory>
#include <stdexcept>

#include "aabb.h"

namespace tracer
{

template <typename T, typename BoxFunc>
    requires requires(T obj)
    {
        { BoxFunc{}(obj) } -> std::convertible_to<AABB>;
    }
class Octree
{
public:
    class Node
    {
        friend class Octree;
    public:
        Node() = default;
        bool IsLeaf() const { return childNodes == nullptr; }
        size_t GetObjectCount() const { return nObj; }
        std::span<const T> GetObjects() const { return std::span<T>(objects.get(), nObj); }
        std::span<const Node, 8> GetChildNodes() const { return std::span<const Node, 8>(childNodes->data(), 8); }
    private:
        Node(const glm::vec3& from, const glm::vec3& to)
            :
            extent(from, to)
        {}
        size_t nObj{};
        std::unique_ptr<T[]> objects;
        std::unique_ptr<std::array<Node, 8>> childNodes; // easier debugging
        AABB extent;
    };
    Octree(size_t nObjPerLeaf, const glm::vec3& from, const glm::vec3& to, const BoxFunc& boxFunc = {})
        : nObjPerLeaf(nObjPerLeaf), topNode(from, to), boxFunc(boxFunc)
    {}
    void Insert(const T& obj)
    {
        insertToNode(&topNode, obj);
    }
    template <typename Func>
        requires requires(Func func, T obj)
        {
            func(obj);
        }
    void Traverse(Func callable) const
    {
        traverseNode(&topNode,
            [](const T* pObj, void* func)
            {
                (*reinterpret_cast<Func*>(func))(*pObj);
            },
            &callable);
    }
    Node* GetTopNode() { return &topNode; }
    const Node* GetTopNode() const { return &topNode; }
private:
    void insertToNode(Node* node, const T& obj)
    {
        glm::vec3 objCenter = boxFunc(std::cref(obj)).GetCenter();
        Node* cur = node;
        do
        {
            if (cur->childNodes) // isn't a leaf node
            {
                bool foundCorrectNode = false;
                for (uint32_t i = 0; i < 8; i++)
                    if (cur->childNodes->at(i).extent.IsInside(objCenter))
                    {
                        cur = &cur->childNodes->at(i);
                        foundCorrectNode = true;
                        break;
                    }
                if (!foundCorrectNode)
                    throw std::runtime_error("obj is outside of the valid extent");
            }
            else if (cur->nObj < nObjPerLeaf)
            {
                if (!cur->objects)
                    allocateObj(cur);
                cur->objects[cur->nObj++] = obj;
                return;
            }
            else
            {
                // split the node
                splitNode(cur);

                // copy the objects over and add the new one
                size_t nObj = cur->nObj + 1;
                std::unique_ptr<T[]> objects = std::make_unique<T[]>(nObj);
                for (uint64_t i = 0; i < cur->nObj; i++)
                    objects[i] = cur->objects[i];
                objects[cur->nObj] = obj;

                cur->objects.reset();
                cur->nObj = 0;

                for (uint64_t i = 0; i < nObj; i++)
                    insertToNode(cur, objects[i]);
                
                return;
            }
        } while (true);
    }
    void allocateObj(Node* node)
    {
        node->objects = std::make_unique<T[]>(nObjPerLeaf);
    }
    void splitNode(Node* node)
    {
        node->childNodes = std::make_unique<std::array<Node, 8>>();
        const AABB& box = node->extent;
        glm::vec3 center = box.GetCenter();
        glm::vec3 min = box.GetMin();
        glm::vec3 max = box.GetMax();
        {
            glm::vec3 p = glm::vec3(min.x, min.y, min.z);
            node->childNodes->at(0).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(max.x, min.y, min.z);
            node->childNodes->at(1).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(max.x, min.y, max.z);
            node->childNodes->at(2).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(min.x, min.y, max.z);
            node->childNodes->at(3).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(min.x, max.y, max.z);
            node->childNodes->at(4).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(max.x, max.y, max.z);
            node->childNodes->at(5).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(max.x, max.y, min.z);
            node->childNodes->at(6).extent = AABB(center, p);
        }
        {
            glm::vec3 p = glm::vec3(min.x, max.y, min.z);
            node->childNodes->at(7).extent = AABB(center, p);
        }
    }

    void traverseNode(const Node* node, void(*func)(const T*, void*), void* userIn) const
    {
        const Node* cur = node;
        if (!cur->childNodes)
        {
            for (uint64_t i = 0; i < cur->nObj; i++)
                (*func)(&cur->objects[i], userIn);
            return;
        }
        for (uint32_t i = 0; i < 8; i++)
            traverseNode(&cur->childNodes->at(i), func, userIn);
    }

    size_t nObjPerLeaf;
    Node topNode;
    BoxFunc boxFunc;
};

}