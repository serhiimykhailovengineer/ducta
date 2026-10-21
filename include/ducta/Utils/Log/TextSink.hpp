/**
 * @file TextSink.hpp
 * @brief Base of the sinks that write one text line per record (see ducta/Utils/Log.hpp):
 *
 *     [12.345678] [warning] iterateNode:233: Error during iteration of node: gain
 *
 * (the time only with Logger::set_time_source()). Derive from TextSink and implement
 * write_line() to send the lines anywhere else (UDP, a display, ...).
 */

#ifndef DUCTA_UTILS_LOG_TEXT_SINK_HPP
#define DUCTA_UTILS_LOG_TEXT_SINK_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Log {

/* Text of one record, without a line break: the message and a prefix such as
 * "[1234567.123456] [warning] function:12345: " */
using Line = FormatBuffer<DUCTA_LOG_MESSAGE_LENGTH + 96>;

/* "[12.345678] [warning] function:42: message" (the time if the record has one) */
inline StringView format_line(Record const& record, Line& line)
{
    char const* const function = record.function != nullptr ? record.function : "";
    int64_t const time_us = record.timestamp.count();
    if (time_us > 0)
    {
        return format_to_n(
            line,
            "[{}.{:06}] [{}] {}:{}: {}",
            time_us / 1000000,
            time_us % 1000000,
            to_string(record.level),
            function,
            record.line,
            record.message);
    }
    return format_to_n(
        line, "[{}] {}:{}: {}", to_string(record.level), function, record.line, record.message);
}

/* A sink writing each record as one line of text */
class TextSink : public Sink
{
public:
    void write(Record const& record) override
    {
        Line line;
        write_line(format_line(record, line), record);
    }

protected:
    /* The line has no line break; record is the one it was formatted from */
    virtual void write_line(StringView line, Record const& record) = 0;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_TEXT_SINK_HPP
