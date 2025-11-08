#ifndef DUCTA_T_OUTPUT_CONCEPT_HPP
#define DUCTA_T_OUTPUT_CONCEPT_HPP

#include "ducta/io/OutputRef.hpp"

namespace ducta {
namespace IO {
namespace Private {

template <typename ValueType>
class TOutputConcept
{
public:
    virtual ~TOutputConcept() = default;

    virtual void set(ValueType const& value) = 0;
    virtual void set(ValueType&& value) = 0;

    virtual OutputRef get_ref() = 0;
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_CONCEPT_HPP