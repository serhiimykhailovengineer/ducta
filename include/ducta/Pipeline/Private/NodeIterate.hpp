#ifndef DUCTA_PIPELINE_PRIVATE_NODEITERATE_HPP
#define DUCTA_PIPELINE_PRIVATE_NODEITERATE_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Utility.hpp"

#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

template <class...>
using void_t = void;

template <class T, class = void>
struct has_iterate : ::ducta::false_type
{
};

template <class T>
struct has_iterate<T, void_t<decltype(::ducta::declval<T&>().iterate())>> : ::ducta::true_type
{
};

template <class T>
constexpr bool has_iterate_v = has_iterate<T>::value;

template <class T, class = void>
struct has_value_error_types : ::ducta::false_type
{
};

template <class T>
struct has_value_error_types<T, void_t<typename T::value_type, typename T::error_type>>
    : ::ducta::true_type
{
};
template <class R, class = void>
struct is_expected_like : ::ducta::false_type
{
};

template <class R>
struct is_expected_like<
    R,
    void_t<
        typename R::value_type,
        typename R::error_type,
        decltype(static_cast<bool>(::ducta::declval<R&>())),
        decltype(::ducta::declval<R&>().error())>> : ::ducta::true_type
{
};

template <class R>
constexpr bool is_expected_like_v = is_expected_like<R>::value;

template <class R, class = void>
struct has_deref : ::ducta::false_type
{
};

template <class R>
struct has_deref<R, void_t<decltype(*::ducta::declval<R&>())>> : ::ducta::true_type
{
};
template <class R, class = void>
struct has_value_fn : ::ducta::false_type
{
};

template <class R>
struct has_value_fn<R, void_t<decltype(::ducta::declval<R&>().value())>> : ::ducta::true_type
{
};

template <class R, class = void>
struct has_message : ::ducta::false_type
{
};

template <class R>
struct has_message<R, void_t<decltype(::ducta::declval<R&>().message)>> : ::ducta::true_type
{
};

template <class R, class = void>
struct has_message_fn : ::ducta::false_type
{
};

template <class R>
struct has_message_fn<R, void_t<decltype(::ducta::declval<R&>().message())>> : ::ducta::true_type
{
};
template <class R, class = void>
struct has_what : ::ducta::false_type
{
};

template <class R>
struct has_what<R, void_t<decltype(::ducta::declval<R&>().what())>> : ::ducta::true_type
{
};

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

/*
 * Result of a node call (init(), iterate()) as Expected<bool, Error>: true to go on, false to
 * stop, or the error. The call may return void (always true), bool, or an expected-like type
 * with value_type void or bool and an error convertible to Error.
 */
template <typename Call>
Expected<bool, Error> call_node(Call&& call)
{
    using R = decltype(call());

    if constexpr (::ducta::is_same_v<R, void>)
    {
        call();
        return true;
    }
    else if constexpr (::ducta::is_same_v<R, bool>)
    {
        return call();
    }
    else if constexpr (is_expected_like_v<R> && ::ducta::is_same_v<typename R::value_type, void>)
    {
        auto r = call();
        if (!r)
        {
            return Unexpected<Error>{convert_to_error(r.error())};
        }
        return true;
    }
    else if constexpr (
        is_expected_like_v<R> && ::ducta::is_same_v<typename R::value_type, bool> &&
        (has_deref<R>::value || has_value_fn<R>::value))
    {
        auto r = call();
        if (!r)
        {
            return Unexpected<Error>{convert_to_error(r.error())};
        }
        return expected_bool_value(r); // true=continue, false=stop
    }
    else
    {
        static_assert(
            sizeof(R) == 0,
            "Unsupported init() / iterate() signature. Use void, bool, or expected-like with "
            "value_type/error_type.");
        return true;
    }
}

template <typename NodeType>
Expected<bool, Error> call_init(NodeType& node)
{
    return call_node([&node]() -> decltype(node.init()) { return node.init(); });
}

template <typename NodeType>
Expected<bool, Error> call_iterate(NodeType& node, Chrono::TimestampUS timestamp)
{
    (void)timestamp;
    if constexpr (!has_iterate_v<NodeType>)
    {
        return true; // no iterate() => always continue
    }
    else
    {
        return call_node([&node]() -> decltype(node.iterate()) { return node.iterate(); });
    }
}

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_NODEITERATE_HPP