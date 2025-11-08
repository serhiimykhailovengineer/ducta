/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_HPP
#define DUCTA_T_INPUT_HPP

#include "ducta/io/Private/TInputModel.hpp"

#include <memory>

namespace ducta {
namespace IO {

/**
 * @brief Type-erased wrapper for input values of type T
 * @tparam T The type of value this input handles
 * 
 * TInput provides a type-erased interface for handling input values,
 * allowing different input implementations to be used polymorphically.
 */
template <typename T>
class TInput
{
public:
    /**
     * @brief Construct TInput from a concrete input type
     * @tparam TInputType The concrete input type
     * @param input The input object to wrap
     */
    template <typename TInputType>
    TInput(TInputType&& input);

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const;

    /**
     * @brief Get the current input value
     * @return Reference to the current value
     */
    T const& value() const;
    
    /**
     * @brief Get the input value or a default if not ready
     * @param default_value The default value to return if not ready
     * @return Reference to the current value or default value
     */
    T const& value_or(T const& default_value) const;

    /**
     * @brief Notify input with a new value
     * @param value The value to notify
     */
    void notify(T const& value);
    
    /**
     * @brief Set a callback to be invoked when value changes
     * @param callback The callback function
     */
    void set_callback(std::function<void(T const&)> callback);

    /**
     * @brief Get a reference to this input
     * @return InputRef reference object
     */
    InputRef get_ref();

    /**
     * @brief Implicit conversion to value type
     * @return Reference to the current value
     */
    operator T const&() const;
    
    /**
     * @brief Check if input is ready (boolean conversion)
     * @return true if value is ready, false otherwise
     */
    operator bool() const;

private:
    std::unique_ptr<Private::TInputConcept<T>> m_input; ///< Type-erased input implementation
};


template <typename T>
template <typename TInputType>
TInput<T>::TInput(TInputType&& input)
: m_input(std::make_unique<Private::TInputModel<std::decay_t<TInputType>, T>>(std::forward<TInputType>(input)))
{
}

template <typename T>
bool TInput<T>::ready() const
{
    return m_input->is_ready();
}

template <typename T>
T const& TInput<T>::value() const
{
    return m_input->get_value();
}

template <typename T>
T const& TInput<T>::value_or(T const& default_value) const
{
    if (ready())
    {
        return value();
    }
    return default_value;
}

template <typename T>
void TInput<T>::notify(T const& value)
{
    m_input->do_notify(value);
}

template <typename T>
void TInput<T>::set_callback(std::function<void(T const&)> callback)
{
    m_input->set_callback(std::move(callback));
}

template <typename T>
InputRef TInput<T>::get_ref()
{
    return m_input->get_ref();
}

template <typename T>
TInput<T>::operator T const&() const
{
    return value();
}

template <typename T>
TInput<T>::operator bool() const
{
    return ready();
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_HPP