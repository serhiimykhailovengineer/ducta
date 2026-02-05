#ifndef DUCTA_CORE_TYPES_TYPEINDEX_HPP
#define DUCTA_CORE_TYPES_TYPEINDEX_HPP

#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/TypeTraits.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

#else
#include <boost/type_index.hpp>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
namespace Private
{
template <class T>
struct TypeAnchor
{
    // This unique address is your "type id" (per type, per binary).
    static constexpr ::ducta::uint8_t anchor = 0;
};
} // namespace Private

struct TypeIndex
{
    const void* v = nullptr;

    // Comparisons
    friend constexpr bool operator==(TypeIndex a, TypeIndex b) noexcept
    {
        return a.v == b.v;
    }

    friend constexpr bool operator!=(TypeIndex a, TypeIndex b) noexcept
    {
        return a.v != b.v;
    }

    friend bool operator<(TypeIndex a, TypeIndex b) noexcept
    {
        return reinterpret_cast<::ducta::uintptr_t>(a.v) < reinterpret_cast<::ducta::uintptr_t>(b.v);
    }

    // Helpers
    constexpr bool valid() const noexcept { return v != nullptr; }

    ::ducta::size_t hash() const noexcept
    {
        return static_cast<::ducta::size_t>(reinterpret_cast<::ducta::uintptr_t>(v));
    }
};

template <typename T>
constexpr TypeIndex type_id() noexcept
{
    return TypeIndex{ &Private::TypeAnchor<remove_cv_t<remove_reference_t<T>>>::anchor };
}

#else
using TypeIndex = boost::typeindex::type_index;

template <typename T>
TypeIndex type_id()
{
    return boost::typeindex::type_id<T>();
}
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_TYPEINDEX_HPP

