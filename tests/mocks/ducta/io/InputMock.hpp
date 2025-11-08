#pragma once

#include <gmock/gmock.h>

// #include "Utils/TypeIndex.hpp"

namespace ducta {
namespace IO {

template <typename T>
class InputMock
{
public:
    MOCK_METHOD(bool, ready, (), (const));
    // MOCK_METHOD(bool, compatible, (Utils::TypeIndex), (const));
    MOCK_METHOD(T const&, value, (), (const));
    MOCK_METHOD(void, notify, (T const& value), ());
};

} // namespace IO
} // namespace ducta