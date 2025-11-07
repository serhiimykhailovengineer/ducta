/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_HPP
#define DUCTA_T_INPUT_HPP

#include "ducta/io/Private/TInputConcept.hpp"

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
private:
    // /**
    //  * @brief Abstract interface for type-erased input concept
    //  */
    // class Concept
    // {
    // public:
    //     virtual ~Concept() = default;
        
    //     /**
    //      * @brief Check if input has a ready value
    //      * @return true if value is ready, false otherwise
    //      */
    //     virtual bool is_ready() const = 0;
        
    //     /**
    //      * @brief Check if input is compatible with given type
    //      * @param type The type index to check compatibility with
    //      * @return true if compatible, false otherwise
    //      */
    //     virtual bool is_compatible(Utils::TypeIndex) const = 0;
        
    //     /**
    //      * @brief Get the current input value
    //      * @return Reference to the current value
    //      */
    //     virtual T const& get_value() const = 0;
        
    //     /**
    //      * @brief Notify input with a new value
    //      * @param value The value to notify
    //      */
    //     virtual void do_notify(T const& value) = 0;
        
    //     /**
    //      * @brief Set a callback to be invoked when value changes
    //      * @param callback The callback function
    //      */
    //     virtual void set_callback(std::function<void(T const&)> callback) = 0;
        
    //     /**
    //      * @brief Get a reference to this input
    //      * @return InputRef reference object
    //      */
    //     virtual InputRef get_ref() = 0;
    // };

    // /**
    //  * @brief Concrete model implementation for specific input types
    //  * @tparam InputT The concrete input type being wrapped
    //  */
    // template <typename InputT>
    // class Model : public Concept
    // {
    //     InputT m_input;

    // public:
    //     Model(InputT input)
    //         : m_input(std::move(input))
    //     {
    //     }

    //     bool is_ready() const override
    //     {
    //         return m_input.ready();
    //     }

    //     bool is_compatible(Utils::TypeIndex type) const override
    //     {
    //         return m_input.compatible(type);
    //     }

    //     T const& get_value() const override
    //     {
    //         return m_input.value();
    //     }

    //     void do_notify(T const& value) override
    //     {
    //         m_input.notify(value);
    //     }

    //     void set_callback(std::function<void(T const&)> callback) override
    //     {
    //         m_input.set_callback(std::move(callback));
    //     }

    //     InputRef get_ref() override
    //     {
    //         return InputRef{m_input};
    //     }
        
    // };

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

} // namespace IO
} // namespace ducta

#include "ducta/io/Private/TInput.inl"

#endif // DUCTA_T_INPUT_HPP