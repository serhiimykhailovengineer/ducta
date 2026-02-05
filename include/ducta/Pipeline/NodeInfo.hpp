#ifndef DUCTA_PIPELINE_NODE_INFO_HPP
#define DUCTA_PIPELINE_NODE_INFO_HPP

#include "ducta/Pipeline/NodeRef.hpp"
#include "ducta/Core/Types/StringView.hpp"

namespace ducta {
namespace Pipeline {

struct NodeInfo
{
    StringView name;
    NodeRef node;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_INFO_HPP