#pragma once

#include <gmock/gmock.h>

#include "ducta/IO/Connection.hpp"

#include "ducta/IO/TInputMock.hpp"
#include "ducta/IO/InputRef.hpp"

namespace ducta {
namespace IO {

template <typename T>
class TOutputMock
{
public:
    MOCK_METHOD(void, set_const_ref, (T const&), ());
    MOCK_METHOD(void, set_move, (T&&), ());

    MOCK_METHOD(Connection, bind, (InputRef& input), ());

};

template <typename T>
inline Connection bind(TOutputMock<T>& output, InputRef& input)
{
    return output.bind(input);
}

} // namespace IO
} // namespace ducta