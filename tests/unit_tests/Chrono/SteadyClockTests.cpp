#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include "ducta/Chrono/SteadyClock.hpp"

using namespace ducta;
using namespace ducta::Chrono;

TEST(SteadyClockTests, epoch_test) 
{
    SteadyClock clock;
    auto epoch = clock.epoch();
    EXPECT_EQ(epoch, SteadyClock::Timestamp{0});
}

TEST(SteadyClockTests, now_test) 
{
    SteadyClock clock;
    auto now = clock.now();
    EXPECT_GE(now, SteadyClock::Timestamp{0});
}

TEST(SteadyClockTests, sleep_for_test) 
{
    SteadyClock clock;
    auto start = clock.now();
    clock.sleepFor(SteadyClock::Timestamp{1000});
    auto end = clock.now();
    EXPECT_GE(end, start + SteadyClock::Timestamp{1000});
}
