#ifndef DUCTA_PIPELINE_NODE_CONCEPT_HPP
#define DUCTA_PIPELINE_NODE_CONCEPT_HPP

#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Map.hpp"

#include <map>

namespace ducta {
namespace Pipeline {
namespace Private {

class NodeConcept
{
public:
    virtual ~NodeConcept() = default;
    virtual void init() = 0;
    virtual Expected<bool, Error> iterate() = 0;
    virtual void release() = 0;

    virtual Map<StringView, IO::InputRef, 25> get_inputs() = 0;
    virtual Map<StringView, IO::OutputRef, 25> get_outputs() = 0;
};
} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_CONCEPT_HPP