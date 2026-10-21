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
 * A sink is any type with
 *     void write(Log::Level level, StringView message, char const* function, uint32_t line,
 *                Chrono::TimestampUS timestamp);
 *     void flush();   // optional: Logger::flush(), e.g. before a reset
 * No base class: the logger keeps the sink's address and one function per sink type (the
 * views passed to write() are valid during the call only). The sink is not owned: it must
 * outlive its attachment.
 *
 * A record is formatted only if some sink accepts its level, into a buffer on the stack of
 * DUCTA_LOG_MESSAGE_LENGTH characters (no heap with either backend, a longer one is truncated).
 * DUCTA_LOG_LEVEL removes the calls of less important levels from the program. Without sinks
 * nothing is written. An application adds its own sinks (UDP, TCP, a queue for another thread,
 * ...).
 *
 * Sinks, each in its own header so that only the used ones are compiled in (ducta/Utils/Log/):
 * ConsoleSink, FileSink, CallbackSink, PrintSink (Arduino), and TextSink (a base formatting one
 * text line per record) for other destinations.
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
#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Utility.hpp"

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

namespace Private {

/* One log message on its way from the macro to the sinks (sinks get its fields) */
struct Record
{
    Level level;
    StringView message;
    char const* function;
    uint32_t line;
    Chrono::TimestampUS timestamp;
};

template <class T, class = void>
struct is_sink : ::ducta::false_type
{
};

template <class T>
struct is_sink<
    T,
    ::ducta::void_t<decltype(::ducta::declval<T&>().write(
        ::ducta::declval<Level>(),
        ::ducta::declval<StringView>(),
        ::ducta::declval<char const*>(),
        ::ducta::declval<uint32_t>(),
        ::ducta::declval<Chrono::TimestampUS>()))>> : ::ducta::true_type
{
};

template <class T, class = void>
struct has_flush : ::ducta::false_type
{
};

template <class T>
struct has_flush<T, ::ducta::void_t<decltype(::ducta::declval<T&>().flush())>> : ::ducta::true_type
{
};

/* The one function of a sink type: writes record, or flushes when it is null */
using SinkFunction = void (*)(void* sink, Record const* record);

template <class SinkType>
void sink_function(void* object, Record const* record)
{
    SinkType& sink = *static_cast<SinkType*>(object);
    if (record != nullptr)
    {
        sink.write(record->level, record->message, record->function, record->line, record->timestamp);
    }
    else
    {
        if constexpr (has_flush<SinkType>::value)
        {
            sink.flush();
        }
    }
}

} // namespace Private

/* Whether SinkType can be attached to the logger (see the contract above) */
template <class SinkType>
constexpr bool is_sink_v = Private::is_sink<SinkType>::value;

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
    template <class SinkType>
    bool add_sink(SinkType& sink, Level min_level = Level::Debug) noexcept
    {
        static_assert(
            is_sink_v<SinkType>,
            "a log sink needs void write(Log::Level, StringView message, char const* function, "
            "uint32_t line, Chrono::TimestampUS timestamp)");
        if (m_count == DUCTA_MAX_LOG_SINKS || find(&sink) != DUCTA_MAX_LOG_SINKS)
        {
            return false;
        }
        m_sinks[m_count] = Entry{&sink, &Private::sink_function<SinkType>};
        m_levels[m_count] = min_level;
        ++m_count;
        update_sink_level();
        return true;
    }

    /* @return false if sink was not attached */
    template <class SinkType>
    bool remove_sink(SinkType& sink) noexcept
    {
        size_t const index = find(&sink);
        if (index == DUCTA_MAX_LOG_SINKS)
        {
            return false;
        }
        for (size_t i = index + 1; i < m_count; ++i)
        {
            m_sinks[i - 1] = m_sinks[i];
            m_levels[i - 1] = m_levels[i];
        }
        --m_count;
        m_sinks[m_count] = Entry{};
        m_levels[m_count] = Level::Off;
        update_sink_level();
        return true;
    }

    /* Detach every sink */
    void clear() noexcept
    {
        for (size_t i = 0; i < DUCTA_MAX_LOG_SINKS; ++i)
        {
            m_sinks[i] = Entry{};
            m_levels[i] = Level::Off;
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

    /* Pass a record to every sink of its level (the DUCTA_LOG_* macros call it) */
    void write(Level level, StringView message, char const* function, uint32_t line)
    {
        if (!enabled(level) || m_writing)
        {
            return; // a sink that logs would recurse
        }
        m_writing = true;
        Private::Record const record{
            level,
            message,
            function != nullptr ? function : "",
            line,
            m_time_source != nullptr ? m_time_source() : Chrono::TimestampUS{0}};
        for (size_t i = 0; i < m_count; ++i)
        {
            if (level >= m_levels[i])
            {
                m_sinks[i].function(m_sinks[i].sink, &record);
            }
        }
        m_writing = false;
    }

    /* Flush every sink that has flush() */
    void flush()
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            m_sinks[i].function(m_sinks[i].sink, nullptr);
        }
    }

private:
    struct Entry
    {
        void* sink{nullptr};
        Private::SinkFunction function{nullptr};
    };

    size_t find(void const* sink) const noexcept
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            if (m_sinks[i].sink == sink)
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
            if (m_levels[i] < m_sink_level)
            {
                m_sink_level = m_levels[i];
            }
        }
    }

    Entry m_sinks[DUCTA_MAX_LOG_SINKS]{};
    Level m_levels[DUCTA_MAX_LOG_SINKS]{};
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

    template <class SinkType>
    bool add_sink(SinkType& /*sink*/, Level /*min_level*/ = Level::Debug) noexcept
    {
        static_assert(
            is_sink_v<SinkType>,
            "a log sink needs void write(Log::Level, StringView, "
            "char const*, uint32_t, Chrono::TimestampUS)");
        return false;
    }

    template <class SinkType>
    bool remove_sink(SinkType& /*sink*/) noexcept
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

    void write(Level /*level*/, StringView /*message*/, char const* /*function*/, uint32_t /*line*/)
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
            ::ducta::Log::logger().write(                                        \
                level,                                                           \
                ::ducta::format_to_n(ducta_log_message_, format, ##__VA_ARGS__), \
                __func__,                                                        \
                static_cast<uint32_t>(__LINE__));                                \
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
