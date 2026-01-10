#include "ducta/Pipeline/Node.hpp"

namespace ducta {
namespace Pipeline {
    
void Node::init()
{
    m_concept->init();
}

bool Node::iterate()
{
    return m_concept->iterate();
}

void Node::release()
{
    m_concept->release();
}

std::map<std::string, IO::InputRef> Node::get_inputs()
{
    return m_concept->get_inputs();
}

std::map<std::string, IO::OutputRef> Node::get_outputs()
{
    return m_concept->get_outputs();
}

} // namespace Pipeline
} // namespace ducta