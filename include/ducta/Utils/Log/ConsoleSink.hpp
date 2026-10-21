/**
 * @file ConsoleSink.hpp
 * @brief Log sink writing to stdout, and warnings and errors to stderr
 */

#ifndef DUCTA_UTILS_LOG_CONSOLE_SINK_HPP
#define DUCTA_UTILS_LOG_CONSOLE_SINK_HPP

#include "ducta/Core/Config.hpp"

#include <cstdio>

#include "ducta/Utils/Log/TextSink.hpp"

namespace ducta {
namespace Log {

/* The console: stdout, and stderr for the records of error_level and above */
class ConsoleSink : public TextSink<ConsoleSink>
{
public:
    explicit ConsoleSink(Level error_level = Level::Warning) noexcept
        : m_error_level{error_level}
    {
    }

    void write_line(StringView line, Level level)
    {
        std::FILE* const out = level >= m_error_level ? stderr : stdout;
        std::fwrite(line.data(), 1, line.size(), out);
        std::fputc('\n', out);
        std::fflush(out); // a console is read as it goes
    }

    void flush()
    {
        std::fflush(stdout);
        std::fflush(stderr);
    }

private:
    Level m_error_level;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_CONSOLE_SINK_HPP
