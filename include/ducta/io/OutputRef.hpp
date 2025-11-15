#ifndef DUCTA_IO_OUTPUT_REF_HPP
#define DUCTA_IO_OUTPUT_REF_HPP

#include "ducta/io/Private/OutputModelRef.hpp"
#include "ducta/io/InputRef.hpp"
#include "ducta/Utils/Deferred.hpp"

#include <memory>

namespace ducta {
namespace IO {

template <typename OutputType>
Utils::Deferred bind(OutputType& output, InputRef& input);

class OutputRef
{
public:
    /**
     * @brief Construct OutputRef from a concrete output type
     * @tparam TOutputType The concrete output type
     * @param output The output object to wrap
     */
    template <typename TOutputType>
    explicit OutputRef(TOutputType&& output)
    : m_output(std::make_shared<Private::OutputModelRef<std::decay_t<TOutputType>>>(std::forward<TOutputType>(output)))
    {}

    Utils::Deferred do_bind(InputRef input)
    {
        return m_output->do_bind(input);
    }

private:
    std::shared_ptr<Private::OutputConcept> m_output; ///< Type-erased output implementation
};

Utils::Deferred bind(OutputRef output, InputRef input)
{
    return output.do_bind(input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_OUTPUT_REF_HPP