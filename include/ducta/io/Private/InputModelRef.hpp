#ifndef DUCTA_INPUT_MODEL_REF_HPP
#define DUCTA_INPUT_MODEL_REF_HPP

#include "ducta/io/Private/InputConcept.hpp"


namespace ducta {
namespace IO {
namespace Private {

template <typename InputT>
class InputModelRef : public InputConcept
{
    InputT& m_input;

public:
    InputModelRef(InputT& input)
        : m_input(input)
    {
    }

    bool is_ready() const override
    {
        return m_input.ready();
    }

    Utils::TypeIndex type_id() const override
    {
        return Utils::type_id<InputT>();
    }

    InputT& get_input()
    {
        return m_input;
    }
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_INPUT_MODEL_REF_HPP