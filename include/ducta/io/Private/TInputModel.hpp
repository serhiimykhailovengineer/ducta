#ifndef DUCTA_T_INPUT_MODEL_HPP
#define DUCTA_T_INPUT_MODEL_HPP

#include "ducta/io/Private/TInputConcept.hpp"

namespace ducta {
namespace IO {
namespace Private {

template <typename InputT, typename ValueType>
class TInputModel : public TInputConcept<ValueType>
{
    InputT m_input;

public:
    TInputModel(InputT input)
        : m_input(std::move(input))
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

    ValueType const& get_value() const override
    {
        return m_input.value();
    }

    void do_notify(ValueType const& value) override
    {
        m_input.notify(value);
    }

    void set_callback(std::function<void(ValueType const&)> callback) override
    {
        m_input.set_callback(std::move(callback));
    }

    InputRef get_ref() override
    {
        return InputRef{m_input};
    }
};

} // namespace Private
} // namespace IO
} // namespace ducta


#endif // DUCTA_T_INPUT_MODEL_HPP