#ifndef DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP
#define DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP

#include "ducta/IO/InputRef.hpp"

#include "ducta/Core/Types/Deferred.hpp"

namespace ducta {
namespace IO {
namespace Private {

class OutputConcept
{
public:
    virtual ~OutputConcept() = default;

    virtual Deferred do_bind(InputRef& input) = 0;
    virtual TypeIndex type_id() const = 0;
    virtual bool areEqual(OutputConcept const& other) const = 0;
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP