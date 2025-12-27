#ifndef DUCTA_PIPELINE_NODE_HPP
#define DUCTA_PIPELINE_NODE_HPP

#include <map>
#include <memory>

#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

namespace ducta {
namespace Pipeline {

namespace Private {
// primary template — assume no iterate() exists
template <typename, typename = std::void_t<>>
struct has_iterate : std::false_type
{
};

// specialization — if T has `void iterate()`
template <typename T>
struct has_iterate<T, std::void_t<decltype(std::declval<T&>().iterate())>> : std::true_type
{
};

template <typename Class>
constexpr bool has_iterate_v = has_iterate<Class>::value;

} // namespace Private

    
class Node
{
private:
    class Concept
    {
    public:
        virtual ~Concept() = default;
        virtual void init() = 0;
        virtual void iterate() = 0;
        virtual void release() = 0;

        virtual std::map<std::string, IO::InputRef> const& get_inputs() = 0;
        virtual std::map<std::string, IO::OutputRef> const& get_outputs() = 0;
    };

    template <typename NodeType>
    class Model : public Concept
    {
    public:
        Model(NodeType&& node)
        : m_node{std::forward<NodeType>(node)}
        {}

        template <typename... Args>
        Model(std::in_place_t, Args&&... args)
        : m_node{std::forward<Args>(args)...}
        {
        }

        void init() override
        {
            m_node.init();
        }

        void iterate() override
        {
        }

        void release() override
        {
            m_node.release();
        }

        std::map<std::string, IO::InputRef> const& get_inputs() override
        {
            return IO::NodeInputsTraits<NodeType>::get(m_node);
        }

        std::map<std::string, IO::OutputRef> const& get_outputs() override
        {
            return IO::NodeOutputsTraits<NodeType>::get(m_node);
        }
    
    private:
        NodeType m_node;
    };

    template <typename NodeType>
    class IterableModel : public Concept
    {
    public:
        IterableModel(NodeType&& node)
        : m_node{std::forward<NodeType>(node)}
        {}

        template <typename... Args>
        IterableModel(std::in_place_t, Args&&... args)
        : m_node{std::forward<Args>(args)...}
        {
        }

        void init() override
        {
            m_node.init();
        }

        void iterate() override
        {
            m_node.iterate();
        }

        void release() override
        {
            m_node.release();
        }

        std::map<std::string, IO::InputRef> const& get_inputs() override
        {
            return IO::NodeInputsTraits<NodeType>::get(m_node);
        }

        std::map<std::string, IO::OutputRef> const& get_outputs() override
        {
            return IO::NodeOutputsTraits<NodeType>::get(m_node);
        }
    
    private:
        NodeType m_node;
    };

    template <typename NodeType>
    std::unique_ptr<Concept> make_node_concept(NodeType&& node)
    {
        using decayed_node_type = std::decay_t<NodeType>;
        if constexpr(Private::has_iterate_v<NodeType>)
        {
            return std::make_unique<IterableModel<decayed_node_type>>(std::forward<NodeType>(node));
        }

        return std::make_unique<Model<decayed_node_type>>(std::forward<NodeType>(node));
    }

    template <typename NodeType, typename... Args>
    std::unique_ptr<Concept> make_node_concept(std::in_place_type_t<NodeType>, Args&&... args)
    {
        using decayed_node_type = std::decay_t<NodeType>;
        if constexpr(Private::has_iterate_v<NodeType>)
        {
            return std::make_unique<IterableModel<decayed_node_type>>(std::in_place_t{}, std::forward<Args>(args)...);
        }

        return std::make_unique<Model<decayed_node_type>>(std::in_place_t{}, std::forward<Args>(args)...);
    }
    
public:
    template <typename NodeType>
    Node(NodeType&& node)
    : m_concept{make_node_concept(std::forward<NodeType>(node))}
    {
    }

    template <typename NodeType, typename... Args>
    Node(std::in_place_type_t<NodeType> in_placer, Args&&... args)
    : m_concept{make_node_concept(in_placer, std::forward<Args>(args)...)}
    {}
    
    void init();
    void iterate();
    void release();

    std::map<std::string, IO::InputRef> const& get_inputs();
    std::map<std::string, IO::OutputRef> const& get_outputs();

private:
    std::unique_ptr<Concept> m_concept;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_HPP