#ifndef DUCTA_PIPELINE_NODEREF_HPP
#define DUCTA_PIPELINE_NODEREF_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Capacity.hpp"

#include "ducta/IO/EventRef.hpp"
#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

#include "ducta/Pipeline/Private/NodeIterate.hpp"

#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"

#include "ducta/Core/Types/TypeRegistry.hpp"
#include "ducta/Pipeline/Private/EventHelper.hpp"

#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Pipeline {

class NodeRef
{
private:
    struct VTable
    {
        Expected<bool, Error> (*init)(void*);
        Expected<bool, Error> (*iterate)(void*, Chrono::TimestampUS);
        void (*release)(void*);
        Map<StringView, IO::InputRef, DUCTA_MAX_NODE_PORTS> (*get_inputs)(void*);
        Map<StringView, IO::OutputRef, DUCTA_MAX_NODE_PORTS> (*get_outputs)(void*);
        Map<StringView, IO::EventRef, DUCTA_MAX_NODE_PORTS> (*get_events)(void*);

        Vector<::ducta::Private::TypeId, DUCTA_MAX_NODE_PORTS> (*get_event_ids)(void*);
        Expected<bool, Error> (*send_event)(void*, ::ducta::Private::TypeId, void*);
    };

    template <class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj) -> Expected<bool, Error> {
                using NodeType = ::ducta::decay_t<T>;
                return Private::call_init(*static_cast<NodeType*>(obj));
            },
            +[](void* obj, Chrono::TimestampUS timestamp) -> Expected<bool, Error> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return Private::call_iterate(node, timestamp);
            },
            +[](void* obj) {
                static_cast<T*>(obj)->release();
            },
            +[](void* obj) -> Map<StringView, IO::InputRef, DUCTA_MAX_NODE_PORTS> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return IO::NodeInputsTraits<NodeType>::get(node);
            },
            +[](void* obj) -> Map<StringView, IO::OutputRef, DUCTA_MAX_NODE_PORTS> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return IO::NodeOutputsTraits<NodeType>::get(node);
            },
            +[](void* obj) -> Map<StringView, IO::EventRef, DUCTA_MAX_NODE_PORTS> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                return IO::NodeEventsTraits<NodeType>::get(node);
            },
            +[](void*) -> Vector<::ducta::Private::TypeId, DUCTA_MAX_NODE_PORTS> {
                using NodeType = ::ducta::decay_t<T>;
                Vector<::ducta::Private::TypeId, DUCTA_MAX_NODE_PORTS> ids;
                for (auto id : Private::event_ids_of(typename SupportedEventsOf<NodeType>::type{}))
                {
                    ids.push_back(id);
                }
                return ids;
            },
            +[](void* obj, ::ducta::Private::TypeId id, void* event) -> Expected<bool, Error> {
                using NodeType = ::ducta::decay_t<T>;
                NodeType& node = *static_cast<NodeType*>(obj);
                static const auto table =
                    Private::make_event_table<NodeType>(typename SupportedEventsOf<NodeType>::type{});
                return Private::call_event(&node, id, event, table);
            }};
        return vt;
    }

public:
    /*
     * @brief Construct a Node from a concrete node type
     * @tparam NodeType The concrete node type
     * @param node The node object to wrap
     * @note This constructor is disabled when NodeType is Node to prevent wrapping a Node in a Node
     */
    template <
        typename NodeType,
        typename = typename ::ducta::enable_if<
            !::ducta::is_same<typename ::ducta::decay<NodeType>::type, NodeRef>::value>::type>
    NodeRef(NodeType& node) noexcept
        : m_node_obj{&node}
        , m_vtable{&vt_for<::ducta::decay_t<NodeType>>()}
    {
    }

    /*
     * @brief Initialize the node
     * @return Expected<bool, Error>: true if the node is ready, false if it cannot run, or the
     * Error it returned. init() of the node may return void, bool or an expected-like type.
     */
    Expected<bool, Error> init()
    {
        return m_vtable->init(m_node_obj);
    }

    /*
     * @brief Perform an iteration of the node
     * @return Expected<bool, Error>: true if iteration was successful and should continue
     * iteration, false if iteration should stop, or an Error if iteration failed
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
     * @brief Send an event to the node
     * @tparam EventType The type of the event
     * @param event The event to send
     * @return Expected<bool, Error>: true if the event was handled successfully,
     *                                       false if the event was not handled, or an Error if handling failed
     */
    template <class EventType>
    Expected<bool, Error> sendEvent(EventType const& event)
    {
        return m_vtable->send_event(
            m_node_obj, DUCTA_TYPE_ID(EventType), const_cast<void*>(static_cast<const void*>(&event)));
    }

    /*
     * @brief Send an already type-erased event to the node
     * @param id Type id of the event (DUCTA_TYPE_ID(EventType))
     * @param event Pointer to the event object of that exact type
     */
    Expected<bool, Error> sendEventRaw(::ducta::Private::TypeId id, void* event)
    {
        return m_vtable->send_event(m_node_obj, id, event);
    }

    /*
     * @brief Get type ids of events the node can receive (its SupportedEvents)
     */
    Vector<::ducta::Private::TypeId, DUCTA_MAX_NODE_PORTS> get_supported_event_ids()
    {
        return m_vtable->get_event_ids(m_node_obj);
    }

    /*
     * @brief Get the inputs of the node
     * @return A map of input names to InputRef objects
     */
    Map<StringView, IO::InputRef, DUCTA_MAX_NODE_PORTS> get_inputs()
    {
        return m_vtable->get_inputs(m_node_obj);
    }

    /*
     * @brief Get the outputs of the node
     * @return A map of output names to OutputRef objects
     */
    Map<StringView, IO::OutputRef, DUCTA_MAX_NODE_PORTS> get_outputs()
    {
        return m_vtable->get_outputs(m_node_obj);
    }

    /*
     * @brief Get the events (emitters) of the node
     */
    Map<StringView, IO::EventRef, DUCTA_MAX_NODE_PORTS> get_events()
    {
        return m_vtable->get_events(m_node_obj);
    }

private:
    void* m_node_obj;
    const VTable* m_vtable;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODEREF_HPP