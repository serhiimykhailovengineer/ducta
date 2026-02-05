/**
 * @file TOutput.hpp
 * @brief Class that represents an output of a specific type.
 */
#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/InputRef.hpp"

#include "ducta/Core/Types/TypeIndex.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/TypeTraits.hpp"

#include "ducta/IO/Connection.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents an output of a specific type. It can bind to inputs of compatible type. Write-only.
 * @tparam T The type of value this output handles. Can be a value type.
 * 
 * This class provides methods to check compatibility with types, set values, and notify inputs of new values.
 */
template <typename T>
class TOutput
{
public:
    using value_type = ::ducta::decay_t<T>;
    using storage_type = value_type;

public:
    /**
     * @brief Construct a TOutput object
     */
    TOutput();

    /** 
     * @brief Set the output value by const reference
     * @param value The value to set
     */
    void set(value_type const& value);

    /** 
     * @brief Set the output value by move
     * @param value The value to set
     */
    void set(value_type&& value);

    /** 
     * @brief Assignment operator to set the output value by const reference
     * @param value The value to set
     * @return Reference to this output
     */
    TOutput<T>& operator=(value_type const& value);

    /** 
     * @brief Assignment operator to set the output value by move
     * @param value The value to set
     * @return Reference to this output
     */
    TOutput<T>& operator=(value_type&& value);

    /** 
     * @brief Bind the output to an input
     * @param input The input to bind to
     * @return A Connection object that will unbind the input upon destruction
     */
    Connection bind(InputRef const& input);

private:
    /** 
     * @brief Unbind the output from an input
     * @param input The input to unbind from
     */
    void unbind(InputRef const& input);

private:
    value_type _value;
    ::ducta::Vector<InputRef, 50> _inputs;
};

template <typename T>
Connection bind(TOutput<T>& output, InputRef const& input)
{
    return output.bind(input);
}

} // namespace IO
} // namespace ducta

#include "ducta/IO/Private/TOutput.inl"

#endif // DUCTA_T_OUTPUT_HPP