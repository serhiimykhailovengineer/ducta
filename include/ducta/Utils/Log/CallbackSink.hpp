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
class CallbackSink : public TextSink
{
public:
    using Callback = void (*)(void* context, StringView line, Record const& record);

    explicit CallbackSink(Callback callback, void* context = nullptr) noexcept
        : m_callback{callback}
        , m_context{context}
    {
    }

protected:
    void write_line(StringView line, Record const& record) override
    {
        if (m_callback != nullptr)
        {
            m_callback(m_context, line, record);
        }
    }

private:
    Callback m_callback;
    void* m_context;
};

} // namespace Log
} // namespace ducta

#endif // DUCTA_UTILS_LOG_CALLBACK_SINK_HPP
