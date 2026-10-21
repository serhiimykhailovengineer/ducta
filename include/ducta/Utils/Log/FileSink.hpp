/**
 * @file FileSink.hpp
 * @brief Log sink writing to a file: one opened (and closed) by the sink, or any std::FILE*
 */

#ifndef DUCTA_UTILS_LOG_FILE_SINK_HPP
#define DUCTA_UTILS_LOG_FILE_SINK_HPP

#include "ducta/Core/Config.hpp"

#include <cstdio>

#include "ducta/Utils/Log/TextSink.hpp"

namespace ducta {
namespace Log {

/* A file: one opened (and closed) by the sink, or an open std::FILE* such as stdout */
class FileSink : public TextSink<FileSink>
{
public:
    /*
     * @brief Write to file, which the sink does not close
     * @param flush_level Records of this level and above are flushed at once (a crash loses
     *        less), the others when the stdio buffer is full or on flush()
     */
    explicit FileSink(std::FILE* file, Level flush_level = Level::Warning) noexcept
        : m_file{file}
        , m_flush_level{flush_level}
    {
    }

    /* Open path, appending to it or replacing it; see is_open() */
    explicit FileSink(char const* path, bool append = true, Level flush_level = Level::Warning) noexcept
        : m_file{std::fopen(path, append ? "a" : "w")}
        , m_flush_level{flush_level}
        , m_owned{true}
    {
    }

    FileSink(FileSink const&) = delete;
    FileSink& operator=(FileSink const&) = delete;

    ~FileSink()
    {
        if (m_owned && m_file != nullptr)
        {
            std::fclose(m_file);
        }
    }

    bool is_open() const noexcept
    {
        return m_file != nullptr;
    }

    void write_line(StringView line, Level level)
    {
        if (m_file == nullptr)
        {
            return;
        }
        std::fwrite(line.data(), 1, line.size(), m_file);
        std::fputc('\n', m_file);
        if (level >= m_flush_level)
        {
            std::fflush(m_file);
        }
    }

    void flush()
    {
        if (m_file != nullptr)
        {
            std::fflush(m_file);
        }
    }

private:
    std::FILE* m_file;
    Level m_flush_level;
    bool m_owned{false};
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_FILE_SINK_HPP
