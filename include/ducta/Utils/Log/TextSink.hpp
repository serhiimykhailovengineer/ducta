/**
 * @file TextSink.hpp
 * @brief Base of the log sinks that write one text line per record (see ducta/Utils/Log.hpp):
 *
 *     [12.345678] [warning] iterateNode:233: Error during iteration of node: gain
 *
 * (the time only with Logger::set_time_source()). A sink derives from TextSink<itself> and
 * implements the line output; flush() is optional:
 *
 *     class UdpSink : public ducta::Log::TextSink<UdpSink>
 *     {
 *     public:
 *         void write_line(ducta::StringView line, ducta::Log::Level level);
 *     };
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

/* "[12.345678] [warning] function:42: message" (the time if there is one) */
inline StringView format_line(
    Line& text,
    Level level,
    StringView message,
    char const* function,
    uint32_t line,
    Chrono::TimestampUS timestamp)
{
    if (function == nullptr)
    {
        function = "";
    }
    if (timestamp.count() > 0)
    {
        // Unsigned: the same formatter as the other numbers (no code for signed 64-bit ones)
        auto const time_us = static_cast<uint64_t>(timestamp.count());
        return format_to_n(
            text,
            "[{}.{:06}] [{}] {}:{}: {}",
            time_us / 1000000,
            time_us % 1000000,
            to_string(level),
            function,
            line,
            message);
    }
    return format_to_n(text, "[{}] {}:{}: {}", to_string(level), function, line, message);
}

/*
 * @brief Formats each record as one line and passes it to Derived::write_line()
 * @tparam Derived The sink, with a public void write_line(StringView line, Level level)
 */
template <class Derived>
class TextSink
{
public:
    void write(Level level, StringView message, char const* function, uint32_t line, Chrono::TimestampUS timestamp)
    {
        Line text;
        static_cast<Derived&>(*this).write_line(
            format_line(text, level, message, function, line, timestamp), level);
    }

protected:
    TextSink() = default;
    ~TextSink() = default;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_TEXT_SINK_HPP
