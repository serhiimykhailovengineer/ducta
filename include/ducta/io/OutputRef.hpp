#ifndef DUCTA_IO_OUTPUT_REF_HPP
#define DUCTA_IO_OUTPUT_REF_HPP

#include "ducta/io/Private/OutputModelRef.hpp"

#include <memory>

namespace ducta {
namespace IO {
class OutputRef
{
public:
    /**
     * @brief Construct OutputRef from a concrete output type
     * @tparam TOutputType The concrete output type
     * @param output The output object to wrap
     */
    template <typename TOutputType>
    OutputRef(TOutputType&& output)
    : m_output(std::make_unique<Private::OutputModelRef<std::decay_t<TOutputType>>>(std::forward<TOutputType>(output)))
    {}

private:
    std::unique_ptr<Private::OutputConcept> m_output; ///< Type-erased output implementation
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_OUTPUT_REF_HPP