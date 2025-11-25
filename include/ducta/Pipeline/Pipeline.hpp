#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include <vector>

#include "ducta/Pipeline/Node.hpp"

namespace ducta {
namespace Pipeline {

class Pipeline
{
private:
    // class Element
    // {
    // public:
    //     virtual ~Element() = default;
    //     virtual void init() = 0;
    //     virtual void stop() = 0;
    // };

    // class IterableElement : public Element
    // {
    // public:
    //     virtual ~IterableElement() = default;
    //     virtual void iterate() = 0;
    // };

    // template <typename T>
    // class ElementWrapper : public Element
    // {
    //     T& m_element;

    // public:
    //     ElementWrapper(T& element)
    //         : m_element(element)
    //     {
    //     }

    //     void init() override { m_element.init(); }
    //     void stop() override { m_element.stop(); }
    // };

    // template <typename T>
    // class IterableElementWrapper : public IterableElement
    // {
    //     T& m_element;

    // public:
    //     IterableElementWrapper(T& element)
    //         : m_element(element)
    //     {
    //     }

    //     void init() override { m_element.init(); }
    //     void iterate() override { m_element.iterate(); }
    //     void stop() override { m_element.stop(); }
    // };

// public:
//     PipelineEngine();
//     ~PipelineEngine();

//     template <typename T>
//     void addElement(T& element);

    void init();
    void iterate();
    void stop();

private:
    std::vector<std::reference_wrapper<Node>> m_nodes;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PIPELINE_HPP