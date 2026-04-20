#pragma once

#include <gmock/gmock.h>

namespace ducta {
namespace Pipeline {

class StopTokenMock
{
public:
    MOCK_METHOD(bool, stopRequested, (), (const));
};

} // namespace Pipeline
} // namespace ducta