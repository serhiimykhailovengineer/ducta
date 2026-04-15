#ifndef DUCTA_PIPELINE_CLOCK_HPP
#define DUCTA_PIPELINE_CLOCK_HPP

#include "ducta/Core/Types/ReferenceWrapper.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Pipeline {

class Clock
{
public:
    using Timestamp = Chrono::TimestampUS;

private:
    struct VTable {
        Timestamp (*now)(void*) ;
        Timestamp (*epoch)(void*) ;
        void (*sleepFor)(void*, Timestamp);
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj) -> Timestamp { return static_cast<T*>(obj)->now(); },
            +[](void* obj) -> Timestamp { return static_cast<T*>(obj)->epoch(); },
            +[](void* obj, Timestamp duration) { static_cast<T*>(obj)->sleepFor(duration); }
        };
        return vt;
    }
    
public:
    template <typename ClockType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<ClockType>, Clock>>>
    Clock(ClockType& clock)
    : m_clock_obj{&clock}
    , m_vtable{&vt_for<ClockType>()}
    {
    }

    Clock(Clock const& other) = default;
    Clock& operator=(Clock const& other) = default;

    Clock(Clock&& other) noexcept = default;
    Clock& operator=(Clock&& other) noexcept = default;

    Timestamp epoch() 
    {
        return m_vtable->epoch(m_clock_obj);
    }
    
    Timestamp now()
    {
        return m_vtable->now(m_clock_obj);
    }

    void sleepFor(Timestamp duration)
    {
        m_vtable->sleepFor(m_clock_obj, duration);
    }

private:
    void* m_clock_obj;
    const VTable* m_vtable;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_CLOCK_HPP