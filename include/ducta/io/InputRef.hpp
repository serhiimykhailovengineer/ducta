/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_REF_HPP
#define DUCTA_T_INPUT_REF_HPP

#include "ducta/io/Private/InputModelRef.hpp"
#include "ducta/Utils/TypeIndex.hpp"

namespace ducta {
namespace IO {

class InputRef
{
public:
    /**
     * @brief Construct TInput from a concrete input type
     * @tparam TInputType The concrete input type
     * @param input The input object to wrap
     */
    template <typename TInputType>
    InputRef(TInputType&& input)
    : m_concept(std::make_unique<Private::InputModelRef<std::decay_t<TInputType>>>(std::forward<TInputType>(input)))
    {}

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const
    {
        return m_concept->is_ready();
    }

    template <typename InputT>
    friend InputT* cast(InputRef& input);

private:
    std::unique_ptr<Private::InputConcept> m_concept; ///< Type-erased input implementation
};

template <typename InputT>
InputT* cast(InputRef& input)
{
    if (Utils::type_id<InputT>() == input.m_concept->type_id())
    {
        InputT& value = static_cast<typename Private::InputModelRef<InputT>*>(input.m_concept.get())->get_input();
        return &value;
    }

    return nullptr;
}

} // namespace io
} // namespace ducta

#endif // DUCTA_T_INPUT_REF_HPP