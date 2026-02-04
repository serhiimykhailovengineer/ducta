#ifndef DUCTA_PIPELINE_NODE_MODEL_HPP
#define DUCTA_PIPELINE_NODE_MODEL_HPP

#include "ducta/Pipeline/Private/NodeConcept.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

template <class...>
using void_t = void;

template <class T, class = void>
struct has_iterate : ::ducta::false_type {};

template <class T>
struct has_iterate<T, void_t<decltype(::ducta::declval<T&>().iterate())>> : ::ducta::true_type {};

template <class T>
constexpr bool has_iterate_v = has_iterate<T>::value;

template <class T, class = void>
struct has_value_error_types : ::ducta::false_type {};

template <class T>
struct has_value_error_types<T, void_t<typename T::value_type, typename T::error_type>> : ::ducta::true_type {};
template <class R, class = void>
struct is_expected_like : ::ducta::false_type {};

template <class R>
struct is_expected_like<R, void_t<
    typename R::value_type,
    typename R::error_type,
    decltype(static_cast<bool>(::ducta::declval<R&>())),
    decltype(::ducta::declval<R&>().error())
>> : ::ducta::true_type {};

template <class R>
constexpr bool is_expected_like_v = is_expected_like<R>::value;

template <class R, class = void>
struct has_deref : ::ducta::false_type {};

template <class R>
struct has_deref<R, void_t<decltype(*::ducta::declval<R&>())>> : ::ducta::true_type {};
template <class R, class = void>
struct has_value_fn : ::ducta::false_type {};

template <class R>
struct has_value_fn<R, void_t<decltype(::ducta::declval<R&>().value())>> : ::ducta::true_type {};

template <class R, class = void>
struct has_message : ::ducta::false_type {};

template <class R>
struct has_message<R, void_t<decltype(::ducta::declval<R&>().message)>> : ::ducta::true_type {};

template <class R, class = void>
struct has_message_fn : ::ducta::false_type {};

template <class R>
struct has_message_fn<R, void_t<decltype(::ducta::declval<R&>().message())>> : ::ducta::true_type {};
template <class R, class = void>
struct has_what : ::ducta::false_type {};

template <class R>
struct has_what<R, void_t<decltype(::ducta::declval<R&>().what())>> : ::ducta::true_type {};

template <class R>
bool expected_bool_value(R& r) noexcept
{
    if constexpr (has_deref<R>::value)
    {
        return static_cast<bool>(*r);
    }
    else
    {
        return static_cast<bool>(r.value());
    }
}

template <class E>
Error convert_to_error(E const& e) noexcept
{
    if constexpr (::ducta::is_same_v<E, Error>)
    {
        return e;
    }
    else if constexpr (has_message<E>::value)
    {
        return Error{e.message};
    }
    else if constexpr (has_message_fn<E>::value)
    {
        return Error{e.message()};
    }
    else if constexpr (has_what<E>::value)
    {
        return Error{String<100>(e.what())};
    }
    else
    {
        static_assert(sizeof(E) == 0, "Cannot convert error type to Utils::Error");
    }
}


template <typename NodeType>
Expected<bool, Error> call_iterate(NodeType& node)
{
    if constexpr (!has_iterate_v<NodeType>)
    {
        return true; // no iterate() => always continue
    }
    else
    {
        using R = decltype(node.iterate());

        if constexpr (::ducta::is_same_v<R, void>)
        {
            node.iterate();
            return true;
        }
        else if constexpr (::ducta::is_same_v<R, bool>)
        {
            return node.iterate();
        }
        else if constexpr (is_expected_like_v<R> && ::ducta::is_same_v<typename R::value_type, void>)
        {
            auto r = node.iterate();
            if (!r)
            {
                return Unexpected<Error>{convert_to_error(r.error())};
            }
            return true;
        }
        else if constexpr (is_expected_like_v<R> && ::ducta::is_same_v<typename R::value_type, bool> &&
                           (has_deref<R>::value || has_value_fn<R>::value))
        {
            auto r = node.iterate();
            if (!r)
            {
                return Unexpected<Error>{convert_to_error(r.error())};
            }
            return expected_bool_value(r); // true=continue, false=stop
        }
        else
        {
            static_assert(sizeof(NodeType) == 0,
                "Unsupported iterate() signature. Use void, bool, or expected-like with value_type/error_type.");
        }
    }
    return true;
}

template <typename NodeType>
class NodeModel : public Private::NodeConcept
{
public:
    NodeModel(NodeType&& node)
    : m_node{::ducta::forward<NodeType>(node)}
    {}

    template <typename... Args>
    NodeModel(std::in_place_t, Args&&... args)
    : m_node{::ducta::forward<Args>(args)...}
    {
    }

    void init() override
    {
        m_node.init();
    }

    Expected<bool, Error> iterate() override
    {
        return Private::call_iterate(m_node);
    }

    void release() override
    {
        m_node.release();
    }

    Map<StringView, IO::InputRef, 25> get_inputs() override
    {
        return IO::NodeInputsTraits<NodeType>::get(m_node);
    }

    Map<StringView, IO::OutputRef, 25> get_outputs() override
    {
        return IO::NodeOutputsTraits<NodeType>::get(m_node);
    }

private:
    NodeType m_node;
};

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_MODEL_HPP