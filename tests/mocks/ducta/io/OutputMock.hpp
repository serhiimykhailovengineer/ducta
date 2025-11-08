#pragma once

#include <gmock/gmock.h>

namespace ducta {
namespace IO {

template <typename T>
class OutputMock
{
public:
    // MOCK_METHOD(Utils::TypeIndex, type_id, (), (const));
    MOCK_METHOD(void, set, (T const& value), ());
};

} // namespace IO
} // namespace ducta
