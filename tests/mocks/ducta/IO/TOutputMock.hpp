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
class TOutputMockWrapper
{
public:
    TOutputMock<T>& mock;

    explicit TOutputMockWrapper(TOutputMock<T>& mock_ref)
    : mock(mock_ref)
    {}

    void set(T const& value)
    {
        mock.set_const_ref(value);
    }

    void set(T&& value)
    {
        mock.set_move(::ducta::move(value));
    }

    Connection bind(InputRef& input)
    {
        return mock.bind(input);
    }
};

template <typename T>
inline Connection bind(TOutputMock<T>& output, InputRef& input)
{
    return output.bind(input);
}

template <typename T>
inline Connection bind(TOutputMockWrapper<T>& output, InputRef& input)
{
    return output.bind(input);
}

} // namespace IO
} // namespace ducta