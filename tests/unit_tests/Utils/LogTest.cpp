#include <gtest/gtest.h>

#include <algorithm>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ducta/Pipeline/Pipeline.hpp"
#include "ducta/Utils/Log.hpp"
#include "ducta/Utils/Log/CallbackSink.hpp"
#include "ducta/Utils/Log/ConsoleSink.hpp"
#include "ducta/Utils/Log/FileSink.hpp"
#include "ducta/Utils/Log/TextSink.hpp"

using namespace ducta;
using Log::Level;

#if DUCTA_ENABLE_LOGGING

namespace {

std::string str(StringView text)
{
    return std::string{text.data(), text.size()};
}

/* Keeps what it receives (a sink: no base class, write() and flush()) */
class RecordingSink
{
public:
    struct Entry
    {
        Level level;
        std::string message;
        std::string function;
        uint32_t line;
        int64_t timestamp;
    };

    void write(Level level, StringView message, char const* function, uint32_t line, Chrono::TimestampUS timestamp)
    {
        records.push_back(Entry{level, str(message), function, line, timestamp.count()});
    }

    void flush()
    {
        ++flushes;
    }

    std::vector<Entry> records;
    int flushes{0};
};

/* Logs from its write(): must not recurse. Has no flush() */
class LoggingSink
{
public:
    void write(Level, StringView, char const*, uint32_t, Chrono::TimestampUS)
    {
        ++writes;
        DUCTA_LOG_ERROR("from a sink");
    }

    int writes{0};
};

struct LogTest : ::testing::Test
{
    void SetUp() override
    {
        reset();
    }

    void TearDown() override
    {
        reset();
    }

    static void reset()
    {
        Log::logger().clear();
        Log::logger().set_level(Level::Info);
        Log::logger().set_time_source(nullptr);
    }
};

} // namespace

TEST_F(LogTest, nothing_is_enabled_without_sinks)
{
    EXPECT_EQ(Log::logger().sink_count(), 0u);
    EXPECT_FALSE(Log::logger().enabled(Level::Error));
    DUCTA_LOG_ERROR("dropped {}", 1); // nothing to check: must not fail
}

TEST_F(LogTest, records_of_enabled_levels_reach_the_sink)
{
    RecordingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));

    DUCTA_LOG_DEBUG("debug {}", 1); // below the default level (info)
    DUCTA_LOG_INFO("info {}", 2);
    DUCTA_LOG_WARNING("warning {}", 3);
    int const line = __LINE__ + 1;
    DUCTA_LOG_ERROR("error {} {}", 4, StringView{"text"});

    ASSERT_EQ(sink.records.size(), 3u);
    EXPECT_EQ(sink.records[0].level, Level::Info);
    EXPECT_EQ(sink.records[0].message, "info 2");
    EXPECT_EQ(sink.records[1].level, Level::Warning);
    EXPECT_EQ(sink.records[2].level, Level::Error);
    EXPECT_EQ(sink.records[2].message, "error 4 text");
    EXPECT_EQ(sink.records[2].function, "TestBody");
    EXPECT_EQ(sink.records[2].line, static_cast<uint32_t>(line));
    EXPECT_EQ(sink.records[2].timestamp, 0);

    Log::logger().set_level(Level::Debug);
    DUCTA_LOG_DEBUG("debug {}", 5);
#if DUCTA_LOG_LEVEL == 0
    ASSERT_EQ(sink.records.size(), 4u);
    EXPECT_EQ(sink.records[3].message, "debug 5");
#else
    EXPECT_EQ(sink.records.size(), 3u); // compiled out by DUCTA_LOG_LEVEL
#endif
}

TEST_F(LogTest, every_sink_gets_its_levels)
{
    RecordingSink all;
    RecordingSink errors;
    ASSERT_TRUE(Log::logger().add_sink(all));
    ASSERT_TRUE(Log::logger().add_sink(errors, Level::Error));

    DUCTA_LOG_INFO("info");
    DUCTA_LOG_ERROR("error");

    EXPECT_EQ(all.records.size(), 2u);
    ASSERT_EQ(errors.records.size(), 1u);
    EXPECT_EQ(errors.records[0].message, "error");
}

TEST_F(LogTest, only_levels_of_some_sink_are_enabled)
{
    RecordingSink errors;
    ASSERT_TRUE(Log::logger().add_sink(errors, Level::Error));
    EXPECT_FALSE(Log::logger().enabled(Level::Warning));
    EXPECT_TRUE(Log::logger().enabled(Level::Error));

    Log::logger().set_level(Level::Off);
    EXPECT_FALSE(Log::logger().enabled(Level::Error));
}

TEST_F(LogTest, sinks_are_attached_once_and_up_to_the_capacity)
{
    RecordingSink sinks[DUCTA_MAX_LOG_SINKS + 1];
    for (size_t i = 0; i < DUCTA_MAX_LOG_SINKS; ++i)
    {
        EXPECT_TRUE(Log::logger().add_sink(sinks[i]));
    }
    EXPECT_FALSE(Log::logger().add_sink(sinks[0])); // already attached
    EXPECT_FALSE(Log::logger().add_sink(sinks[DUCTA_MAX_LOG_SINKS])); // full

    EXPECT_TRUE(Log::logger().remove_sink(sinks[0]));
    EXPECT_FALSE(Log::logger().remove_sink(sinks[0]));
    EXPECT_TRUE(Log::logger().add_sink(sinks[DUCTA_MAX_LOG_SINKS]));

    DUCTA_LOG_INFO("to every attached sink");
    EXPECT_TRUE(sinks[0].records.empty());
    for (size_t i = 1; i <= DUCTA_MAX_LOG_SINKS; ++i)
    {
        EXPECT_EQ(sinks[i].records.size(), 1u) << i;
    }
}

TEST_F(LogTest, removed_sink_receives_nothing)
{
    RecordingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink, Level::Info));
    ASSERT_TRUE(Log::logger().remove_sink(sink));
    EXPECT_FALSE(Log::logger().enabled(Level::Error));
    DUCTA_LOG_ERROR("dropped");
    EXPECT_TRUE(sink.records.empty());
}

TEST_F(LogTest, a_sink_that_logs_does_not_recurse)
{
    LoggingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));
    DUCTA_LOG_ERROR("first");
    EXPECT_EQ(sink.writes, 1);
}

static_assert(Log::is_sink_v<RecordingSink>, "write(level, message, function, line, timestamp)");
static_assert(Log::is_sink_v<Log::CallbackSink>, "TextSink provides write()");
static_assert(!Log::is_sink_v<int>, "not a sink");

TEST_F(LogTest, sink_without_flush_is_skipped_by_flush)
{
    LoggingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));
    Log::logger().flush(); // must not call write()
    EXPECT_EQ(sink.writes, 0);
}

TEST_F(LogTest, flush_reaches_every_sink)
{
    RecordingSink a;
    RecordingSink b;
    ASSERT_TRUE(Log::logger().add_sink(a));
    ASSERT_TRUE(Log::logger().add_sink(b, Level::Error));
    Log::logger().flush();
    EXPECT_EQ(a.flushes, 1);
    EXPECT_EQ(b.flushes, 1);
}

TEST_F(LogTest, time_source_sets_the_timestamp)
{
    RecordingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));
    Log::logger().set_time_source(+[] {
        return Chrono::TimestampUS{12345678};
    });
    DUCTA_LOG_INFO("timed");
    ASSERT_EQ(sink.records.size(), 1u);
    EXPECT_EQ(sink.records[0].timestamp, 12345678);
}

TEST_F(LogTest, long_message_is_truncated)
{
    RecordingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));
    std::string const text(DUCTA_LOG_MESSAGE_LENGTH + 50, 'x');
    DUCTA_LOG_INFO("{}", StringView{text.data(), text.size()});
    ASSERT_EQ(sink.records.size(), 1u);
    EXPECT_EQ(sink.records[0].message.size(), static_cast<size_t>(DUCTA_LOG_MESSAGE_LENGTH));
}

TEST_F(LogTest, long_line_is_truncated)
{
    std::string const text(Log::Line::capacity + 10, 'y');
    Log::Line line;
    StringView const view = Log::format_line(
        line, Level::Info, StringView{text.data(), text.size()}, "f", 1, Chrono::TimestampUS{0});
    EXPECT_EQ(view.size(), Log::Line::capacity);
    EXPECT_EQ(str(view).substr(0, 12), "[info] f:1: ");
}

TEST_F(LogTest, line_format)
{
    Log::Line line;
    Log::format_line(line, Level::Warning, "message", "function", 42, Chrono::TimestampUS{0});
    EXPECT_EQ(str(line.view()), "[warning] function:42: message");

    Log::format_line(line, Level::Error, "late", "f", 7, Chrono::TimestampUS{3000123});
    EXPECT_EQ(str(line.view()), "[3.000123] [error] f:7: late");
}

TEST_F(LogTest, callback_sink_receives_lines)
{
    std::vector<std::string> lines;
    Log::CallbackSink sink{
        [](void* context, StringView line, Level) {
            static_cast<std::vector<std::string>*>(context)->push_back(str(line));
        },
        &lines};
    ASSERT_TRUE(Log::logger().add_sink(sink));
    int const line = __LINE__ + 1;
    DUCTA_LOG_WARNING("value {}", 7);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "[warning] TestBody:" + std::to_string(line) + ": value 7");
}

TEST_F(LogTest, file_sink_writes_lines)
{
    std::string const path = ::testing::TempDir() + "ducta_log_test.txt";
    {
        Log::FileSink sink{path.c_str(), false};
        ASSERT_TRUE(sink.is_open());
        ASSERT_TRUE(Log::logger().add_sink(sink));
        DUCTA_LOG_INFO("first");
        DUCTA_LOG_ERROR("second {}", 2);
        Log::logger().remove_sink(sink);
    } // closed

    std::ifstream file{path};
    std::string first;
    std::string second;
    std::getline(file, first);
    std::getline(file, second);
    EXPECT_NE(first.find("[info] "), std::string::npos) << first;
    EXPECT_NE(first.find(": first"), std::string::npos) << first;
    EXPECT_NE(second.find("[error] "), std::string::npos) << second;
    EXPECT_NE(second.find(": second 2"), std::string::npos) << second;
    std::remove(path.c_str());
}

TEST_F(LogTest, file_sink_of_missing_directory_is_not_open)
{
    Log::FileSink sink{"/nonexistent-directory/ducta.log"};
    EXPECT_FALSE(sink.is_open());
    ASSERT_TRUE(Log::logger().add_sink(sink));
    DUCTA_LOG_ERROR("dropped"); // must not crash
    Log::logger().clear();
}

TEST_F(LogTest, pipeline_reports_through_the_logger)
{
    RecordingSink sink;
    ASSERT_TRUE(Log::logger().add_sink(sink));

    struct Clock
    {
        using Timestamp = Chrono::TimestampUS;
        Timestamp epoch()
        {
            return Timestamp{0};
        }
        Timestamp now()
        {
            return Timestamp{0};
        }
        void sleepFor(Timestamp)
        {
        }
    } clock;
    Pipeline::Nodes nodes;
    Pipeline::Pipeline pipeline{clock};
    Pipeline::Pipeline::Config config;
    config.order.push_back("missing");
    EXPECT_FALSE(pipeline.configure(nodes, config)); // the overload that logs its diagnostics

    // The pipeline may log information too: the error is checked, not the first record
    auto const error = std::find_if(sink.records.begin(), sink.records.end(), [](auto const& record) {
        return record.level == Level::Error;
    });
    ASSERT_NE(error, sink.records.end());
    EXPECT_NE(error->message.find("missing"), std::string::npos) << error->message;
}

#endif // DUCTA_ENABLE_LOGGING
