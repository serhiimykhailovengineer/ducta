#ifndef DUCTA_PIPELINE_NODEREF_HPP
#define DUCTA_PIPELINE_NODEREF_HPP

#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

#include "ducta/Pipeline/Private/NodeIterate.hpp"

#include "ducta/Core/Types/Expected.hpp"
#include "ducta/Core/Types/Error.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/TypeTraits.hpp"

#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Pipeline {

class NodeRef
{
private:
    struct VTable {
        void (*init)(void*);
        Expected<bool, Error> (*iterate)(void*, Chrono::TimestampUS);
        void (*release)(void*);
        Map<StringView, IO::InputRef, 25> (*get_inputs)(void*);
        Map<StringView, IO::OutputRef, 25> (*get_outputs)(void*);
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj) {
                static_cast<T*>(obj)->init();
            },
            +[](void* obj, Chrono::TimestampUS timestamp) -> Expected<bool, Error> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return Private::call_iterate(node, timestamp);
            },
            +[](void* obj) {
                static_cast<T*>(obj)->release();
            },
            +[](void* obj) -> Map<StringView, IO::InputRef, 25> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return IO::NodeInputsTraits<NodeType>::get(node);
            },
            +[](void* obj) -> Map<StringView, IO::OutputRef, 25> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return IO::NodeOutputsTraits<NodeType>::get(node);
            }
        };
        return vt;
    }
    
public:
    /*
    * @brief Construct a Node from a concrete node type
    * @tparam NodeType The concrete node type
    * @param node The node object to wrap
    * @note This constructor is disabled when NodeType is Node to prevent wrapping a Node in a Node
    */
    template <typename NodeType, 
              typename = typename ::ducta::enable_if<!::ducta::is_same<typename ::ducta::decay<NodeType>::type, NodeRef>::value>::type>
    NodeRef(NodeType& node) noexcept
    : m_node_obj{&node}
    , m_vtable{&vt_for<::ducta::decay_t<NodeType>>()}
    {
    }

    /*
     * @brief Initialize the node
    */
    void init()
    {
        m_vtable->init(m_node_obj);
    }

    /*
     * @brief Perform an iteration of the node
     * @return Expected<bool, Error>: true if iteration was successful and should continue iteration, 
     *                                       false if iteration should stop, or an Error if iteration failed
    */
    Expected<bool, Error> iterate(Chrono::TimestampUS timestamp = Chrono::TimestampUS{0})
    {
        return m_vtable->iterate(m_node_obj, timestamp);
    }

    /*
     * @brief Release the node
    */
    void release()
    {
        m_vtable->release(m_node_obj);
    }

    /*
     * @brief Get the inputs of the node
     * @return A map of input names to InputRef objects
    */
    Map<StringView, IO::InputRef, 25> get_inputs()
    {
        return m_vtable->get_inputs(m_node_obj);
    }

    /*
     * @brief Get the outputs of the node
     * @return A map of output names to OutputRef objects
    */
    Map<StringView, IO::OutputRef, 25> get_outputs()
    {
        return m_vtable->get_outputs(m_node_obj);
    }

private:
    void* m_node_obj;
    const VTable* m_vtable;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODEREF_HPP