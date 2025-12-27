/**
 * @file OutputRef.inl
 * @brief Type-erased output wrapper for handling output values
 */

#ifndef DUCTA_IO_OUTPUT_REF_INL
#define DUCTA_IO_OUTPUT_REF_INL

#include "ducta/IO/OutputRef.hpp"

#include <memory>

namespace ducta {
namespace IO {

Utils::Deferred OutputRef::do_bind(InputRef input)
{
    return m_output->do_bind(input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_OUTPUT_REF_INL