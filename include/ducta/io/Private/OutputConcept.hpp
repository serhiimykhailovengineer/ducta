#ifndef DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP
#define DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP

#include "ducta/io/InputRef.hpp"

#include "ducta/Utils/Deferred.hpp"

namespace ducta {
namespace IO {
namespace Private {

class OutputConcept
{
public:
    virtual ~OutputConcept() = default;

    virtual Utils::Deferred do_bind(InputRef& input) = 0;
};

} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_PRIVATE_OUTPUT_CONCEPT_HPP