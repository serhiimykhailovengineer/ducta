/**
 * @file Log.hpp
 * @brief Logging: records go to the sinks attached to the logger (console, file, Serial, ...)
 *
 *     ducta::Log::ConsoleSink console;                    // ducta/Utils/Log/ConsoleSink.hpp
 *     ducta::Log::logger().add_sink(console);             // every record of the enabled levels
 *     ducta::Log::logger().add_sink(file, ducta::Log::Level::Warning); // warnings and errors only
 *     ducta::Log::logger().set_level(ducta::Log::Level::Debug);
 *
 *     DUCTA_LOG_INFO("node '{}' created", name);
 *
 * A record is formatted only if some sink accepts its level, into a buffer on the stack of
 * DUCTA_LOG_MESSAGE_LENGTH characters (no heap with either backend, a longer one is truncated).
 * DUCTA_LOG_LEVEL removes the calls of less important levels from the program. Without sinks
 * nothing is written. A sink is anything implementing Log::Sink: an application adds its own (UDP,
 * TCP, a queue for another thread, ...).
 *
 * Sinks, each in its own header so that only the used ones are compiled in (ducta/Utils/Log/):
 * ConsoleSink, FileSink, CallbackSink, PrintSink (Arduino), and TextSink to write lines elsewhere.
 *
 * Without logging (DUCTA_ENABLE_LOGGING 0; CMake -DDUCTA_ENABLE_LOGGING=OFF) the DUCTA_LOG_*
 * calls compile to nothing, their arguments are not evaluated, and the logger keeps its API but
 * does nothing (add_sink() returns false). A sink created by the application is still compiled
 * in: create it under #if DUCTA_ENABLE_LOGGING.
 *
 * @note The logger is not thread-safe yet: log from one thread, or from sinks that tolerate it.
 */

#ifndef DUCTA_UTILS_LOG_HPP
#define DUCTA_UTILS_LOG_HPP

#include "ducta/Core/Config.hpp"

// No <iostream>: before GCC 13 it adds a static std::ios_base::Init to every file, which links
// the iostreams and locales of libstdc++ (~120 KB of flash on a microcontroller)

#include "ducta/Chrono/Chrono.hpp"
#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"

namespace ducta {
namespace Log {

/* Importance of a record; Off disables a sink or the logger */
enum class Level : uint8_t
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Off = 4
};

constexpr StringView to_string(Level level) noexcept
{
    switch (level)
    {
    case Level::Debug:
        return "debug";
    case Level::Info:
        return "info";
    case Level::Warning:
        return "warning";
    case Level::Error:
        return "error";
    case Level::Off:
        break;
    }
    return "off";
}

/* One log message; the views are valid only during Sink::write() */
struct Record
{
    Level level;
    StringView message;
    /* Function that logged (__func__) and its line */
    char const* function;
    uint32_t line;
    /* Of the logger's time source; 0 without one */
    Chrono::TimestampUS timestamp;
};

/* Destination of records. Not owned by the logger: it must outlive its attachment */
class Sink
{
public:
    virtual ~Sink() = default;

    virtual void write(Record const& record) = 0;

    /* Write out what is buffered (Logger::flush(), e.g. before a reset) */
    virtual void flush()
    {
    }
};

#if DUCTA_ENABLE_LOGGING

class Logger
{
public:
    using TimeSource = Chrono::TimestampUS (*)();

    constexpr Logger() = default;

    Logger(Logger const&) = delete;
    Logger& operator=(Logger const&) = delete;

    /*
     * @brief Attach sink for the records of min_level and above
     * @return false if it is attached already or DUCTA_MAX_LOG_SINKS sinks are
     */
    bool add_sink(Sink& sink, Level min_level = Level::Debug) noexcept
    {
        if (m_count == DUCTA_MAX_LOG_SINKS || find(sink) != DUCTA_MAX_LOG_SINKS)
        {
            return false;
        }
        m_sinks[m_count++] = Entry{&sink, min_level};
        update_sink_level();
        return true;
    }

    /* @return false if sink was not attached */
    bool remove_sink(Sink& sink) noexcept
    {
        size_t const index = find(sink);
        if (index == DUCTA_MAX_LOG_SINKS)
        {
            return false;
        }
        for (size_t i = index + 1; i < m_count; ++i)
        {
            m_sinks[i - 1] = m_sinks[i];
        }
        m_sinks[--m_count] = Entry{};
        update_sink_level();
        return true;
    }

    /* Detach every sink */
    void clear() noexcept
    {
        for (Entry& entry : m_sinks)
        {
            entry = Entry{};
        }
        m_count = 0;
        update_sink_level();
    }

    size_t sink_count() const noexcept
    {
        return m_count;
    }

    /* Records below level are dropped before they are formatted (default: Info) */
    void set_level(Level level) noexcept
    {
        m_level = level;
    }

    Level level() const noexcept
    {
        return m_level;
    }

    /* Timestamps of the records, e.g. +[] { return SteadyClock{}.now(); } (default: none) */
    void set_time_source(TimeSource time_source) noexcept
    {
        m_time_source = time_source;
    }

    /* Whether a record of level would be written by some sink */
    bool enabled(Level level) const noexcept
    {
        return level != Level::Off && level >= m_level && level >= m_sink_level;
    }

    /* Pass record to every sink of its level; its timestamp is set from the time source */
    void write(Record record)
    {
        if (!enabled(record.level) || m_writing)
        {
            return; // a sink that logs would recurse
        }
        m_writing = true;
        if (m_time_source != nullptr)
        {
            record.timestamp = m_time_source();
        }
        for (size_t i = 0; i < m_count; ++i)
        {
            if (record.level >= m_sinks[i].min_level)
            {
                m_sinks[i].sink->write(record);
            }
        }
        m_writing = false;
    }

    void flush()
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            m_sinks[i].sink->flush();
        }
    }

private:
    struct Entry
    {
        Sink* sink{nullptr};
        Level min_level{Level::Off};
    };

    size_t find(Sink const& sink) const noexcept
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            if (m_sinks[i].sink == &sink)
            {
                return i;
            }
        }
        return DUCTA_MAX_LOG_SINKS;
    }

    /* The least important level accepted by a sink: below it nothing is formatted */
    void update_sink_level() noexcept
    {
        m_sink_level = Level::Off;
        for (size_t i = 0; i < m_count; ++i)
        {
            if (m_sinks[i].min_level < m_sink_level)
            {
                m_sink_level = m_sinks[i].min_level;
            }
        }
    }

    Entry m_sinks[DUCTA_MAX_LOG_SINKS]{};
    size_t m_count{0};
    Level m_level{Level::Info};
    Level m_sink_level{Level::Off};
    TimeSource m_time_source{nullptr};
    bool m_writing{false};
};

#else // DUCTA_ENABLE_LOGGING

/* Logging disabled: the same interface, doing nothing */
class Logger
{
public:
    using TimeSource = Chrono::TimestampUS (*)();

    constexpr Logger() = default;

    Logger(Logger const&) = delete;
    Logger& operator=(Logger const&) = delete;

    bool add_sink(Sink& /*sink*/, Level /*min_level*/ = Level::Debug) noexcept
    {
        return false;
    }

    bool remove_sink(Sink& /*sink*/) noexcept
    {
        return false;
    }

    void clear() noexcept
    {
    }

    size_t sink_count() const noexcept
    {
        return 0;
    }

    void set_level(Level /*level*/) noexcept
    {
    }

    Level level() const noexcept
    {
        return Level::Off;
    }

    void set_time_source(TimeSource /*time_source*/) noexcept
    {
    }

    bool enabled(Level /*level*/) const noexcept
    {
        return false;
    }

    void write(Record const& /*record*/)
    {
    }

    void flush()
    {
    }
};

#endif // DUCTA_ENABLE_LOGGING

/* The logger of the program (constant-initialized: usable before main()) */
inline Logger& logger() noexcept
{
    static Logger instance;
    return instance;
}

} // namespace Log
} // namespace ducta

namespace ducta {
namespace Log {
namespace Private {

/* Uses the arguments of a log call that is compiled out (no unused-variable warnings) */
template <class... Args>
constexpr void discard(Args const&... /*args*/) noexcept
{
}

} // namespace Private
} // namespace Log
} // namespace ducta

/*
 * DUCTA_LOG_<LEVEL>(format, args...): format as fmt::format / etl::format ("{}" placeholders)
 * and pass to the sinks of the level
 */
#define DUCTA_LOG(level, format, ...)                                            \
    do                                                                           \
    {                                                                            \
        if (::ducta::Log::logger().enabled(level))                               \
        {                                                                        \
            ::ducta::FormatBuffer<DUCTA_LOG_MESSAGE_LENGTH> ducta_log_message_;  \
            ::ducta::Log::logger().write(::ducta::Log::Record{                   \
                level,                                                           \
                ::ducta::format_to_n(ducta_log_message_, format, ##__VA_ARGS__), \
                __func__,                                                        \
                static_cast<uint32_t>(__LINE__),                                 \
                ::ducta::Chrono::TimestampUS{0}});                               \
        }                                                                        \
    } while (false)

/* A call compiled out: nothing is evaluated, the arguments only count as used */
#define DUCTA_LOG_NOTHING(format, ...)                             \
    do                                                             \
    {                                                              \
        if (false)                                                 \
        {                                                          \
            ::ducta::Log::Private::discard(format, ##__VA_ARGS__); \
        }                                                          \
    } while (false)

#if DUCTA_ENABLE_LOGGING && DUCTA_LOG_LEVEL <= 0
#define DUCTA_LOG_DEBUG(format, ...) DUCTA_LOG(::ducta::Log::Level::Debug, format, ##__VA_ARGS__)
#else
#define DUCTA_LOG_DEBUG(format, ...) DUCTA_LOG_NOTHING(format, ##__VA_ARGS__)
#endif

#if DUCTA_ENABLE_LOGGING && DUCTA_LOG_LEVEL <= 1
#define DUCTA_LOG_INFO(format, ...) DUCTA_LOG(::ducta::Log::Level::Info, format, ##__VA_ARGS__)
#else
#define DUCTA_LOG_INFO(format, ...) DUCTA_LOG_NOTHING(format, ##__VA_ARGS__)
#endif

#if DUCTA_ENABLE_LOGGING && DUCTA_LOG_LEVEL <= 2
#define DUCTA_LOG_WARNING(format, ...) \
    DUCTA_LOG(::ducta::Log::Level::Warning, format, ##__VA_ARGS__)
#else
#define DUCTA_LOG_WARNING(format, ...) DUCTA_LOG_NOTHING(format, ##__VA_ARGS__)
#endif

#if DUCTA_ENABLE_LOGGING && DUCTA_LOG_LEVEL <= 3
#define DUCTA_LOG_ERROR(format, ...) DUCTA_LOG(::ducta::Log::Level::Error, format, ##__VA_ARGS__)
#else
#define DUCTA_LOG_ERROR(format, ...) DUCTA_LOG_NOTHING(format, ##__VA_ARGS__)
#endif

#endif // DUCTA_UTILS_LOG_HPP
