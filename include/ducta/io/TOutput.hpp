/**
 * @file TOutput.hpp
 * @brief Class that represents an output of a specific type.
 */
#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/InputRef.hpp"

#include "ducta/Utils/TypeIndex.hpp"
#include "ducta/Utils/Deferred.hpp"

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
    using value_type = std::decay_t<T>;
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
     * @return A Deferred object that will unbind the input upon destruction
     */
    Utils::Deferred bind(InputRef const& input);

private:
    value_type _value;
    std::vector<InputRef> _inputs;
};

template <typename T>
Utils::Deferred bind(TOutput<T>& output, InputRef const& input)
{
    return output.bind(input);
}

// template <typename T>
// TOutput<T>::TOutput()
// : _value{}
// {
// }

// template <typename T>
// Utils::TypeIndex TOutput<T>::type_id() const
// {
//     return Utils::type_id<T>();
// }

// template <typename T>
// void TOutput<T>::set(value_type const& value) 
// {
//     _value = value;
//     for (auto& input_ref : _inputs)
//     {
//         input_ref.notify(_value);
//     }
// }

// template <typename T>
// void TOutput<T>::set(value_type&& value) 
// {
//     _value = std::move(value);
//     for (auto& input_ref : _inputs)
//     {
//         input_ref.notify(_value);
//     }
// }

// template <typename T>
// TOutput<T>& TOutput<T>::operator=(value_type const& value)
// {
//     set(value);
//     return *this;
// }

// template <typename T>
// TOutput<T>& TOutput<T>::operator=(value_type&& value)
// {
//     set(std::move(value));
//     return *this;
// }

// template <typename T>
// Utils::Deferred TOutput<T>::bind(InputRef const& input)
// {
//     if (input.compatible(Utils::type_id<T>()))
//     {
//         _inputs.emplace_back(input);
//         return Utils::Deferred{[this, input = std::move(input)]() {
//             // Remove input from the list upon destruction
//             _inputs.erase(std::remove_if(_inputs.begin(), _inputs.end(),
//                 [&input](InputRef& ref) { return ref == input; }),
//                 _inputs.end());
//         }};
//     }

//     return {};
// }

} // namespace IO
} // namespace ducta

#include "ducta/IO/Private/TOutput.inl"

#endif // DUCTA_T_OUTPUT_HPP