#ifndef DUCTA_INPUT_CONCEPT_HPP
#define DUCTA_INPUT_CONCEPT_HPP

namespace ducta {
namespace IO {
namespace Private {
class InputConcept
{
public:
    virtual ~InputConcept() = default;
    virtual bool is_ready() const = 0;
};
} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_INPUT_CONCEPT_HPP