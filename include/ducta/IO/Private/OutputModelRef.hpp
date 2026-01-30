#ifndef DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP
#define DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP

#include "ducta/IO/Private/OutputConcept.hpp"

namespace ducta {
namespace IO {

namespace Private {

template <typename OutputT>
class OutputModelRef : public OutputConcept
{
    OutputT& m_output;

public:
    OutputModelRef(OutputT& output) 
    : m_output(output) 
    {}

    Deferred do_bind(InputRef& input) override
    {
        return bind(m_output, input);
    }

    TypeIndex type_id() const override
    {
        return ::ducta::type_id<OutputT>();
    }

    bool areEqual(OutputConcept const& other) const override
    {
        if (::ducta::type_id<OutputT>() != other.type_id())
            return false;

        auto const& other_model = static_cast<OutputModelRef<OutputT> const&>(other);
        return &m_output == &other_model.m_output;
    }
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP