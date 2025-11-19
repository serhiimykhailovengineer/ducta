#ifndef DUCTA_VALUEIO_VALUE_IO_BUILDER_HPP
#define DUCTA_VALUEIO_VALUE_IO_BUILDER_HPP

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"

#include "ducta/ValueIO/TValueInput.hpp"
#include "ducta/ValueIO/TValueOutput.hpp"

namespace ducta {
namespace IO {

class ValueIOBuilder
{
public:
    template<typename ValueType>
    IO::TInput<ValueType> make_input()
    {
        return IO::TInput<ValueType>{std::in_place_type_t<IO::TValueInput<ValueType>>{}};
    }

    template<typename ValueType>
    IO::TOutput<ValueType> make_output()
    {
        return IO::TOutput<ValueType>{std::in_place_type_t<IO::TValueOutput<ValueType>>{}};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_VALUEIO_VALUE_IO_BUILDER_HPP