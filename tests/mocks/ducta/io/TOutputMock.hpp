#pragma once

#include <gmock/gmock.h>

// #include "ducta/Utils/TypeIndex.hpp"
#include "ducta/io/TInputMock.hpp"
#include "ducta/io/InputRef.hpp"

namespace ducta {
namespace IO {

template <typename T>
class TOutputMock
{
public:
    MOCK_METHOD(void, set_const_ref, (T const&), ());
    MOCK_METHOD(void, set_move, (T&&), ());

    // MOCK_METHOD(Utils::Deferred, bind, (TInputMockWrapper<T>& input), ());

};

template <typename T>
class TOutputMockWrapper
{
public:
    TOutputMock<T>& mock;

    void set(T const& value)
    {
        mock.set_const_ref(value);
    }

    void set(T&& value)
    {
        mock.set_move(std::move(value));
    }
};

// template <typename T>
// inline Utils::Deferred bind(TOutputMockWrapper<T>& output, TInputMockWrapper<T>& input)
// {
//     return output.mock.bind(input);
// }

// template <typename T>
// inline Utils::Deferred bind(TOutputMockWrapper<T>& output, InputRef& input)
// {
//     if(auto wrapper = cast<TInputMockWrapper<T>>(input))
//     {
//         return bind(output, *wrapper);
//     }
//     throw std::runtime_error("Incompatible types for binding TOutputMockWrapper");

//     return {};
// }
} // namespace IO
} // namespace ducta