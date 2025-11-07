/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_REF_HPP
#define DUCTA_T_INPUT_REF_HPP

#include "ducta/io/Private/InputConcept.hpp"

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
    InputRef(TInputType&& input);

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const;

private:
    std::unique_ptr<Private::InputConcept> m_input; ///< Type-erased input implementation
};

} // namespace io
} // namespace ducta

#include "ducta/io/Private/InputRef.inl"

#endif // DUCTA_T_INPUT_REF_HPP