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

    bool is_compatible(Utils::TypeIndex type) const override
    {
        return type == Utils::type_id<typename InputT::value_type>();
    }

    void notify(Utils::TypeIndex type, void const* value) override
    {
        using ValueType = typename InputT::value_type;
        if (type != Utils::type_id<ValueType>())
        {
            throw std::runtime_error("Incompatible type for notification");
        }

        m_input.notify(*static_cast<ValueType const*>(value));
    }

    Utils::TypeIndex type_id() const override
    {
        return Utils::type_id<InputT>();
    }

    bool areEqual(InputConcept const& other) const override
    {
        if (Utils::type_id<InputT>() != other.type_id())
            return false;

        auto const& other_model = static_cast<InputModelRef<InputT> const&>(other);
        return &m_input == &other_model.m_input;
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