#ifndef DUCTA_PIPELINE_CLOCK_HPP
#define DUCTA_PIPELINE_CLOCK_HPP

#include <map>
#include <memory>

namespace ducta {
namespace Pipeline {

class Clock
{
public:
    using Timestamp = uint64_t;

private:
    class Concept
    {
    public:
        virtual ~Concept() = default;
        virtual Timestamp epoch() = 0;
        virtual Timestamp now() = 0;
    };

    template <typename ClockType>
    class Model : public Concept
    {
    public:
        Model(ClockType& clock)
        : m_clock{clock}
        {}

        Timestamp epoch() override
        {
            return m_clock.get().epoch();
        }

        Timestamp now() override
        {
            return m_clock.get().now();
        }
    private:
        std::reference_wrapper<ClockType> m_clock;
    };
    
public:
    template <typename ClockType>
    Clock(ClockType& clock)
    : m_concept{std::make_shared<Model<ClockType>>(clock)}
    {
    }

    Timestamp epoch() 
    {
        return m_concept->epoch();
    }
    
    Timestamp now()
    {
        return m_concept->now();
    }

private:
    std::shared_ptr<Concept> m_concept;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_CLOCK_HPP