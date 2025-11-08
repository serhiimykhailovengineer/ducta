#ifndef DUCTA_INPUT_CONCEPT_HPP
#define DUCTA_INPUT_CONCEPT_HPP

#include "ducta/Utils/TypeIndex.hpp"

namespace ducta {
namespace IO {
namespace Private {
class InputConcept
{
public:
    virtual ~InputConcept() = default;
    virtual bool is_ready() const = 0;
    virtual Utils::TypeIndex type_id() const = 0;
};
} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_INPUT_CONCEPT_HPP