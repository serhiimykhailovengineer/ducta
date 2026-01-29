#ifndef DUCTA_PIPELINE_NODE_CONCEPT_HPP
#define DUCTA_PIPELINE_NODE_CONCEPT_HPP

#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

#include "ducta/Utils/Error.hpp"
#include "ducta/Utils/Expected.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

class NodeConcept
{
public:
    virtual ~NodeConcept() = default;
    virtual void init() = 0;
    virtual Utils::Expected<bool, Utils::Error> iterate() = 0;
    virtual void release() = 0;

    virtual std::map<std::string, IO::InputRef> get_inputs() = 0;
    virtual std::map<std::string, IO::OutputRef> get_outputs() = 0;
};
} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_CONCEPT_HPP