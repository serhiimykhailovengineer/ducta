#include "ducta/Pipeline/Pipeline.hpp"

namespace ducta {
namespace Pipeline {


void Pipeline::init()
{
    for (auto& node : m_nodes)
    {
        node.get().init();
    }
}

void Pipeline::iterate()
{
    for (auto& node : m_nodes)
    {
        node.get().iterate();
    }
}

void Pipeline::stop()
{
    for (auto& node : m_nodes)
    {
        node.get().release();
    }
}

} // namespace Pipeline
} // namespace ducta