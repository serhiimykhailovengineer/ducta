#ifndef DUCTA_T_OUTPUT_MODEL_HPP
#define DUCTA_T_OUTPUT_MODEL_HPP

#include "ducta/io/Private/TOutputConcept.hpp"

namespace ducta {
namespace IO {
namespace Private {

template <typename OutputT, typename ValueType>
class TOutputModel : public TOutputConcept<ValueType>
{
    OutputT m_output;

public:
    TOutputModel(OutputT output)
        : m_output(std::move(output))
    {
    }

    template <typename... Args>
    TOutputModel(std::in_place_t, Args&&... args)
        : m_output(std::forward<Args>(args)...)
    {
    }

    void set(ValueType const& value) override
    {
        m_output.set(value);
    }

    void set(ValueType&& value) override
    {
        m_output.set(std::move(value));
    }

    OutputRef get_ref() override
    {
        return OutputRef{m_output};
    }
};

} // namespace Private
} // namespace IO
} // namespace ducta


#endif // DUCTA_T_OUTPUT_MODEL_HPP