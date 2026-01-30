/**
 * @file InputRef.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_REF_HPP
#define DUCTA_T_INPUT_REF_HPP

#include "ducta/IO/Private/InputModelRef.hpp"
#include "ducta/Core/Types/TypeIndex.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include <memory>

namespace ducta {
namespace IO {

/**
 * @brief Class that represents a type-erased reference of input. Read-only.
 * 
 */
class InputRef
{
public:
    /**
     * @brief Construct InputRef from a concrete input type
     * @tparam TInputType The concrete input type
     * @param input The input object to wrap
     */
    template <typename TInputType,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<TInputType>, InputRef>>>
    explicit InputRef(TInputType& input)
        : m_concept(std::make_shared<Private::InputModelRef<std::decay_t<TInputType>>>(input))
    {}

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const
    {
        return m_concept->is_ready();
    }

    /**
     * @brief Check if the input is compatible with the given type
     * @param type The type to check compatibility against
     * @return true if compatible, false otherwise
     */
    bool compatible(TypeIndex type) const
    {
        return m_concept->is_compatible(type);
    }

    /**
     * @brief Notify the input of a new value
     * @tparam T The type of the value
     * @param value The value to notify
     */
    template <typename T>
    void notify(T const& value)
    {
        m_concept->notify(type_id<std::decay_t<T>>(), &value);
    }

    /**
     * @brief Attempt to cast the InputRef back to its original concrete type
     * @tparam InputT The concrete input type to cast to
     * @return Pointer to the concrete input type if successful, nullptr otherwise
     */
    template <typename InputT>
    InputT* as()
    {
        if (type_id<InputT>() != m_concept->type_id())
            return nullptr;

        using ModelRefT = Private::InputModelRef<InputT>;
        auto* model = static_cast<ModelRefT*>(m_concept.get());
        return &model->get_input();
    }

    /**
    * @brief Friend function to cast InputRef to concrete type
    * @tparam InputT The concrete input type to cast to
    * @param input The InputRef to cast
    * @return Pointer to the concrete input type if successful, nullptr otherwise
    */
    template <typename InputT>
    friend InputT* cast(InputRef input);

    /**
     * @brief Equality operator for InputRef
     * @param lhs Left-hand side InputRef
     * @param rhs Right-hand side InputRef
     * @return true if both InputRefs refer to the same underlying input, false otherwise
     */
    friend bool operator==(InputRef const& lhs, InputRef const& rhs)
    {
        return lhs.m_concept->areEqual(*rhs.m_concept);
    }

private:
    std::shared_ptr<Private::InputConcept> m_concept; ///< Type-erased input implementation
};

template <typename InputT>
InputT* cast(InputRef input)
{
    return input.as<InputT>();
}

template <typename Node>
struct NodeInputsTraits
{
    static Map<StringView, IO::InputRef, 25> get(Node& node)
    {
        return {};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_REF_HPP