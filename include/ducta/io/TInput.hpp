/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_HPP
#define DUCTA_T_INPUT_HPP

#include <boost/optional.hpp>

#include "ducta/Utils/TypeIndex.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents an input of a specific type. It can be bind to output of compatible type. Read-only.
 * @tparam T The type of value this input handles. Can be a value type or a reference type.
 * 
 * This class provides methods to check compatibility with types, check readiness, retrieve values, and notify new values.
 */
template <typename T>
class TInput
{
public:
    using value_type = std::decay_t<T>;
    using storage_type = std::conditional_t<std::is_reference<T>::value, const value_type&, value_type>;

public:
    TInput() = default;
    explicit TInput(T const& value)
        : _value(value)
    {
    }

    bool compatible(Utils::TypeIndex type) const
    {
        return type == Utils::type_id<T>();
    }

    bool ready() const
    {
        return _value.has_value();
    }

    storage_type value()
    {
        if (!ready())
        {
            throw std::runtime_error("Input not ready");
        }

        storage_type result_value = _value.value();
        _value.reset();
        return result_value;
    }

    storage_type value_or(value_type const& default_value)
    {
        storage_type result_value = _value.value_or(default_value);
        _value.reset();
        return result_value;
    }

    void notify(T const& value)
    {
        _value.emplace(value);
    }

    void release()
    {
        _value.reset();
    }

    operator bool() const
    {
        return ready();
    }

private:
    boost::optional<storage_type> _value;
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_HPP