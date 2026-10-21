/**
 * @file PrintSink.hpp
 * @brief Log sink writing to an Arduino Print: Serial, Serial1, a TCP client (WiFiClient,
 *        EthernetClient), ...
 */

#ifndef DUCTA_UTILS_LOG_PRINT_SINK_HPP
#define DUCTA_UTILS_LOG_PRINT_SINK_HPP

#include "ducta/Core/Config.hpp"

#if !defined(ARDUINO)
#error "ducta/Utils/Log/PrintSink.hpp needs the Arduino framework"
#endif

#include <Arduino.h>

#include "ducta/Utils/Log/TextSink.hpp"

namespace ducta {
namespace Log {

/* An Arduino Print: Serial, Serial1, a TCP client (WiFiClient, EthernetClient), ... */
class PrintSink : public TextSink
{
public:
    explicit PrintSink(Print& out) noexcept
        : m_out{out}
    {
    }

    void flush() override
    {
        m_out.flush();
    }

protected:
    void write_line(StringView line, Record const& /*record*/) override
    {
        m_out.write(reinterpret_cast<uint8_t const*>(line.data()), line.size());
        m_out.write(static_cast<uint8_t>('\r'));
        m_out.write(static_cast<uint8_t>('\n'));
    }

private:
    Print& m_out;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_PRINT_SINK_HPP
