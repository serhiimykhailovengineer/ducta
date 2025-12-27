#ifndef DUCTA_IO_OUTPUT_REF_HPP
#define DUCTA_IO_OUTPUT_REF_HPP

#include "ducta/IO/Private/OutputModelRef.hpp"
#include "ducta/IO/InputRef.hpp"
#include "ducta/Utils/Deferred.hpp"

#include <memory>

namespace ducta {
namespace IO {

/**
 * @brief Class that represents a type-erased reference of output.
 * 
 */
class OutputRef
{
public:
    /**
     * @brief Construct OutputRef from a concrete output type
     * @tparam TOutputType The concrete output type
     * @param output The output object to wrap
     */
    template <typename TOutputType,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<TOutputType>, OutputRef>>>
    explicit OutputRef(TOutputType& output)
    : m_output(std::make_shared<Private::OutputModelRef<std::decay_t<TOutputType>>>(output))
    {}

    OutputRef(OutputRef const&) = default;
    OutputRef& operator=(OutputRef const&) = default;

    Utils::Deferred do_bind(InputRef input)
    {
        return m_output->do_bind(input);
    }

    friend bool operator==(OutputRef const& lhs, OutputRef const& rhs)
    {
        return lhs.m_output->areEqual(*rhs.m_output);
    }

private:
    std::shared_ptr<Private::OutputConcept> m_output; ///< Type-erased output implementation
};

inline Utils::Deferred bind(OutputRef output, InputRef input)
{
    return output.do_bind(input);
}

template <typename Node>
struct NodeOutputsTraits
{
    static std::map<std::string, IO::OutputRef> get(Node& node)
    {
        return std::map<std::string, IO::OutputRef>{};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_OUTPUT_REF_HPP