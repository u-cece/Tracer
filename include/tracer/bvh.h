#pragma once

#include <ranges>
#include <vector>

#include "octree.h"

namespace tracer
{

template <typename T, typename BoxFunc>
    requires requires(T obj)
    {
        { BoxFunc{}(obj) } -> std::convertible_to<AABB>;
    }
class BVH
{
    using OctreeType = Octree<T, BoxFunc>;
    using OctreeNode = OctreeType::Node;
public:
    class Node
    {
        friend class BVH;
    public:
        Node() = default;
        bool IsLeaf() const { return childNodes == nullptr; }
        const T& GetObject() const { return object; }
        std::span<const Node, 2> GetChildNodes() const { return std::span<const Node, 2>(childNodes->data(), 2); }
        AABB GetExtent() const { return extent; }
    private:
        size_t objCount{};
        T object{};
        std::unique_ptr<std::array<Node, 2>> childNodes;
        AABB extent;
    };
    BVH(const BoxFunc& boxFunc = {})
        : boxFunc(boxFunc)
    {
    }
    template <std::ranges::input_range Range>
        requires
            std::is_same_v<
                std::ranges::range_value_t<Range>,
                T>
    BVH(Range&& objects, const BoxFunc& boxFunc = {})
        : boxFunc(boxFunc)
    {
        Build(objects);
    }
    template <std::ranges::input_range Range>
        requires
            std::is_same_v<
                std::ranges::range_value_t<Range>,
                T>
    void Build(Range&& objects)
    {
        AABB extent = calcExtent(objects);

        OctreeType octree(2, extent.GetMin(), extent.GetMax(), boxFunc);
        for (const T& obj : objects)
            octree.Insert(obj);

        topNode = buildFromNode(octree.GetTopNode());
    }
    bool IsBuilt() const { return topNode != nullptr; }
private:
    template <typename T>
    struct optionalValueType : std::false_type {};
    template <typename T>
    struct optionalValueType<std::optional<T>> : std::true_type
    {
        using type = T;
    };
public:
    template <typename IntersectionFunc, typename DistanceFunc,
        typename Result =
            optionalValueType<
                std::invoke_result_t<
                    IntersectionFunc,
                    T, // object
                    glm::vec3, glm::vec3 // ray
                >
            >::type,
        typename Distance = std::invoke_result_t<DistanceFunc, Result>>
        requires requires(IntersectionFunc intersectionFunc)
        {
            intersectionFunc(T(), glm::vec3(), glm::vec3());
        } && requires(DistanceFunc distanceFunc, Result result)
        {
            distanceFunc(result);
        }
    std::optional<Result> Intersect(
        const glm::vec3& orig, const glm::vec3& dir,
        const IntersectionFunc& intersectionFunc = {},
        const DistanceFunc& distanceFunc = {}) const
    {
        auto result = intersectNode<IntersectionFunc, DistanceFunc, Result, Distance>(
            topNode.get(), orig, dir, intersectionFunc, distanceFunc);
        if (result)
            return result.value().second;
        return std::nullopt;
    }
private:
    template <typename IntersectionFunc, typename DistanceFunc, typename Result, typename Distance>
    std::optional<std::pair<Distance, Result>> intersectNode(
        const Node* cur,
        const glm::vec3& orig, const glm::vec3& dir,
        const IntersectionFunc& intersectionFunc,
        const DistanceFunc& distanceFunc) const
    {
        if (!cur->extent.IsInside(orig) && !cur->extent.Intersect(orig, dir))
            return std::nullopt;
        if (cur->childNodes)
        {
            auto leftResult = intersectNode<IntersectionFunc, DistanceFunc, Result, Distance>(
                getLeftNode(cur), orig, dir, intersectionFunc, distanceFunc);
            auto rightResult = intersectNode<IntersectionFunc, DistanceFunc, Result, Distance>(
                getRightNode(cur), orig, dir, intersectionFunc, distanceFunc);
            if (!leftResult && !rightResult)
                return std::nullopt;
            if (leftResult && !rightResult)
                return leftResult;
            if (!leftResult && rightResult)
                return rightResult;
            return std::min(leftResult, rightResult, [](const auto& a, const auto& b) { return a.value().first < b.value().first; });
        }
        std::optional<Result> result = intersectionFunc(cur->object, orig, dir);
        if (!result)
            return std::nullopt;
        return std::optional<std::pair<Distance, Result>>(std::make_pair(distanceFunc(result.value()), result.value()));
    }
    AABB calcExtent(auto&& objects)
    {
        glm::vec3 min;
        glm::vec3 max;
        bool first = true;
        for (const T& obj : objects)
        {
            AABB box = boxFunc(obj);
            glm::vec3 objMin = box.GetMin();
            glm::vec3 objMax = box.GetMax();
            if (first)
            {
                min = objMin;
                max = objMax;
                first = false;
                continue;
            }
            for (uint32_t i = 0; i < 3; i++)
                min[i] = glm::min(min[i], objMin[i]);
            for (uint32_t i = 0; i < 3; i++)
                max[i] = glm::max(max[i], objMax[i]);
        }
        return AABB(min, max);
    }
    std::unique_ptr<Node> buildFromNode(const OctreeNode* cur)
    {
        if (cur->IsLeaf())
        {
            std::span<const T> objects = cur->GetObjects();
            size_t nObj = objects.size();
            if (nObj == 0)
                return nullptr;

            std::unique_ptr<Node> node = std::make_unique<Node>();
            if (nObj == 1)
                setObjectNode(node.get(), objects[0]);
            else if (nObj == 2)
            {
                allocChildNodes(node.get());
                setObjectNode(getLeftNode(node.get()), objects[0]);
                setObjectNode(getRightNode(node.get()), objects[1]);
                node->objCount = 2;
                node->extent = AABB(boxFunc(objects[0]), boxFunc(objects[1]));
            }
            else
                std::unreachable();

            return node;
        }

        std::vector<std::unique_ptr<Node>> nodes;
        for (uint32_t i = 0; i < 8; i++)
        {
            std::span<const OctreeNode, 8> childNodes = cur->GetChildNodes();
            std::unique_ptr<Node> node = buildFromNode(&childNodes[i]);
            if (node)
                nodes.push_back(std::move(node));
        }
        
        if (nodes.size() == 0)
            return nullptr;
        if (nodes.size() == 1)
            return std::move(nodes.front());

        // to-do: try SAH
        return groupNodes(std::make_move_iterator(nodes.begin()), nodes.size());
    }
    std::unique_ptr<Node> groupNodes(std::move_iterator<typename std::vector<std::unique_ptr<Node>>::iterator> begin, size_t length)
    {
        // to-do: replace recursion with bottom-up
        if (length > 2)
        {
            uint64_t partition = length / 2;
            std::unique_ptr<Node> node = std::make_unique<Node>();
            allocChildNodes(node.get());
            Node* left = getLeftNode(node.get());
            Node* right = getRightNode(node.get());
            *left = std::move(*groupNodes(begin, partition));
            *right = std::move(*groupNodes(begin + partition, length - partition));
            node->objCount = left->objCount + right->objCount;
            node->extent = AABB(left->extent, right->extent);
            return node;
        }
        if (length == 1)
            return *begin;
        if (length == 2)
        {
            std::unique_ptr<Node> node = std::make_unique<Node>();
            allocChildNodes(node.get());
            Node* left = getLeftNode(node.get());
            Node* right = getRightNode(node.get());
            *left = std::move(**begin);
            *right = std::move(**(begin + 1));
            node->objCount = left->objCount + right->objCount;
            node->extent = AABB(left->extent, right->extent);
            return node;
        }
        std::unreachable();
    }
    static void allocChildNodes(Node* node)
    {
        node->childNodes = std::make_unique<std::array<Node, 2>>();
    }
    static Node* getLeftNode(Node* node)
    {
        return &node->childNodes->at(0);
    }
    static const Node* getLeftNode(const Node* node)
    {
        return &node->childNodes->at(0);
    }
    static Node* getRightNode(Node* node)
    {
        return &node->childNodes->at(1);
    }
    static const Node* getRightNode(const Node* node)
    {
        return &node->childNodes->at(1);
    }
    void setObjectNode(Node* node, const T& obj)
    {
        node->object = obj;
        node->objCount = 1;
        node->extent = boxFunc(obj);
    }

    std::unique_ptr<Node> topNode;
    BoxFunc boxFunc;
};

}