/**
 * @file CallbackSink.hpp
 * @brief Log sink passing each line to a function of the application: a UART driver, an RTOS
 *        queue, a UDP socket (lwIP, WiFiUDP), ...
 */

#ifndef DUCTA_UTILS_LOG_CALLBACK_SINK_HPP
#define DUCTA_UTILS_LOG_CALLBACK_SINK_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Utils/Log/TextSink.hpp"

namespace ducta {
namespace Log {

/* Each line goes to a function, with a context pointer of the application */
class CallbackSink : public TextSink<CallbackSink>
{
public:
    /* line has no line break */
    using Callback = void (*)(void* context, StringView line, Level level);

    explicit CallbackSink(Callback callback, void* context = nullptr) noexcept
        : m_callback{callback}
        , m_context{context}
    {
    }

    void write_line(StringView line, Level level)
    {
        if (m_callback != nullptr)
        {
            m_callback(m_context, line, level);
        }
    }

private:
    Callback m_callback;
    void* m_context;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_CALLBACK_SINK_HPP
