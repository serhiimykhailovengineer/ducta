// Compiled with DUCTA_ENABLE_LOGGING=0 (and -Werror=unused): the log calls disappear

#include <gtest/gtest.h>

#include <string>

#include "ducta/Utils/Log.hpp"
#include "ducta/Utils/Log/CallbackSink.hpp"

static_assert(DUCTA_ENABLE_LOGGING == 0, "this test checks the build without logging");

using namespace ducta;
using Log::Level;

namespace {

int evaluations = 0;

int counted(int value)
{
    ++evaluations;
    return value;
}

} // namespace

TEST(LogDisabledTest, calls_are_not_evaluated)
{
    int const only_logged = 42; // used by the log calls only: no unused-variable warning
    DUCTA_LOG_DEBUG("debug {}", counted(only_logged));
    DUCTA_LOG_INFO("info {}", counted(1));
    DUCTA_LOG_WARNING("warning {}", counted(2));
    DUCTA_LOG_ERROR("error {} {}", counted(3), StringView{"text"});
    DUCTA_LOG_ERROR("no arguments");
    EXPECT_EQ(evaluations, 0);
}

TEST(LogDisabledTest, logger_does_nothing)
{
    int lines = 0;
    Log::CallbackSink sink{
        [](void* context, StringView, Log::Record const&) {
            ++*static_cast<int*>(context);
        },
        &lines};
    EXPECT_FALSE(Log::logger().add_sink(sink));
    EXPECT_EQ(Log::logger().sink_count(), 0u);
    EXPECT_FALSE(Log::logger().enabled(Level::Error));
    Log::logger().set_level(Level::Debug);
    EXPECT_EQ(Log::logger().level(), Level::Off);
    DUCTA_LOG_ERROR("dropped");
    Log::logger().write(Log::Record{Level::Error, "dropped", "f", 1, Chrono::TimestampUS{0}});
    EXPECT_EQ(lines, 0);

    // A sink still works when used directly
    sink.write(Log::Record{Level::Error, "direct", "f", 1, Chrono::TimestampUS{0}});
    EXPECT_EQ(lines, 1);
}
