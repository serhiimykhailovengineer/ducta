#ifndef DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP
#define DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP

#include "ducta/io/Private/OutputConcept.hpp"

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

    Utils::Deferred do_bind(InputRef input) override
    {
        return m_output.bind(input); // return bind(m_output, input);
    }
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_PRIVATE_OUTPUT_MODEL_REF_HPP