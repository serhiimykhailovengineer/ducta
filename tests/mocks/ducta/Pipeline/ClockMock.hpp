#pragma once

#include <gmock/gmock.h>

namespace ducta {
namespace Pipeline {

class ClockMock
{
    using Timestamp = std::uint64_t;

public:
    MOCK_METHOD(Timestamp, epoch, (), ());
    MOCK_METHOD(Timestamp, now, (), ());
};

} // namespace Pipeline
} // namespace ducta