#ifndef DUCTA_CORE_TYPES_TYPEREGISTRY_HPP
#define DUCTA_CORE_TYPES_TYPEREGISTRY_HPP

#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Core/Algorithms/Format.hpp"

#include "ducta/Core/Assert.hpp"

namespace ducta {

namespace Private {

template <class T>
constexpr StringView auto_type_name()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr StringView sig = __PRETTY_FUNCTION__;
    constexpr size_t prefix = sig.find("T = ") + 4;
    constexpr size_t suffix = sig.find_last_of(']');
    return sig.substr(prefix, suffix - prefix);
#elif defined(_MSC_VER)
    constexpr StringView sig = __FUNCSIG__;
    constexpr size_t prefix = sig.find("auto_type_name<") + 16;
    constexpr size_t suffix = sig.find_last_of('>');
    return sig.substr(prefix, suffix - prefix);
#endif
}

using TypeId = uint32_t;

struct TypeFingerprint
{
    size_t size;
    size_t align;
};

struct TypeRegistryEntry
{
    TypeId id;
    TypeFingerprint fingerprint;
    StringView registered_at_file;
    int registered_at_line;
};

class TypeRegistry
{
public:
    static TypeRegistry& instance()
    {
        static TypeRegistry reg;
        return reg;
    }

    TypeId register_or_get(StringView type_name, TypeFingerprint fingerprint, StringView file, int line)
    {
        // LockGuard lock(m_mutex);

        auto it = m_entries.find(type_name);
        if (it != m_entries.end())
        {
            TypeRegistryEntry* existing = &it->second;

            if (existing->fingerprint.size != fingerprint.size ||
                existing->fingerprint.align != fingerprint.align)
            {
                String<200> Error_msg;
                format_to(
                    Error_msg,
                    "Event type name collision: '{}' first registered at {}:{} "
                    "(size={}, align={}), now requested with size={}, align={} at {}:{}",
                    type_name.data(),
                    existing->registered_at_file.data(),
                    existing->registered_at_line,
                    static_cast<unsigned long long>(existing->fingerprint.size),
                    static_cast<unsigned long long>(existing->fingerprint.align),
                    static_cast<unsigned long long>(fingerprint.size),
                    static_cast<unsigned long long>(fingerprint.align),
                    file.data(),
                    line);
                // Assert::fail(Error_msg);
            }
            return existing->id;
        }

        TypeId id = m_next_id++;
        m_entries.insert(
            ::ducta::pair<StringView, TypeRegistryEntry>(
                ::ducta::move(type_name), TypeRegistryEntry{id, fingerprint, file, line}));
        return id;
    }

    /* Registered name of the type, or an empty view if the id is unknown. */
    StringView name_of(TypeId id) const
    {
        for (auto const& entry : m_entries)
        {
            if (entry.second.id == id)
            {
                return entry.first;
            }
        }
        return StringView{};
    }

private:
    // Mutex m_mutex;
    Map<StringView, TypeRegistryEntry, 50> m_entries;
    TypeId m_next_id = 1;
};
} // namespace Private

} // namespace ducta

#endif // DUCTA_CORE_TYPES_TYPEREGISTRY_HPP