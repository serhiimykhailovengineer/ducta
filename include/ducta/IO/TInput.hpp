/**
 * @file TInput.hpp
 * @brief Class that represents an input of a specific type.
 */

#ifndef DUCTA_T_INPUT_HPP
#define DUCTA_T_INPUT_HPP

#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/TypeIndex.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents an input of a specific type. It can be bound to output of compatible type. Read-only.
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
    using storage_holder = std::conditional_t<std::is_reference<T>::value, ::ducta::OptionalRef<value_type>, ::ducta::Optional<storage_type>>;

public:
    TInput() = default;

    /**
     * @brief Check if the input is compatible with the given type
     * @param type The type to check compatibility against
     * @return true if compatible, false otherwise
     */
    bool compatible(TypeIndex type) const;

    /**
     * @brief Check if the input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const;

    /**
     * @brief Retrieve the value of the input. Consumes the value.
     * @return The value of the input
     * @throws std::runtime_error if the input is not ready
     */
    storage_type value();

    /**
     * @brief Retrieve the value of the input or return a default value if not ready. Consumes the value if ready.
     * @param default_value The default value to return if input is not ready
     * @return The value of the input or the default value
     */
    storage_type value_or(value_type const& default_value);

    /**
     * @brief Notify the input of a new value
     * @param value The new value to notify
     */
    void notify(T const& value);

    /**
     * @brief Release the current value, marking the input as not ready
     */
    void release();

    /**
     * @brief Check if the input is ready (boolean context)
     * @return true if ready, false otherwise
     */
    operator bool() const;

private:
    storage_holder _value;
};

} // namespace IO
} // namespace ducta

#include "ducta/IO/Private/TInput.inl"

#endif // DUCTA_T_INPUT_HPP