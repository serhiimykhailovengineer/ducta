#ifndef DUCTA_VALUEIO_TVALUE_INPUT_HPP
#define DUCTA_VALUEIO_TVALUE_INPUT_HPP

#include "ducta/Utils/TypeIndex.hpp"
// #include "ducta/io/Value/Binder.hpp"

#include <functional>
#include <boost/optional.hpp>

namespace ducta {
namespace IO {

template <typename T>
class TValueInput
{
public:
    using value_type = T;

public:
    TValueInput() = default;
    explicit TValueInput(T const& value)
        : _value(value)
    {
    }

    bool compatible(Utils::TypeIndex type) const
    {
        return type == Utils::type_id<T>();
    }

    bool ready() const
    {
        return _value.has_value(); 
    }

    T const& value() const
    {
        if (!ready())
        {
            throw std::runtime_error("Input not ready");
        }
        return _value.value();
    }

    T const& value_or(T const& default_value) const
    {
        return _value.value_or(default_value);
    }

    void notify(T const& value)
    {
        _value.emplace(value);
    }

    void set_callback(std::function<void(T const&)> callback)
    {
        // No-op for TValueInput
    }

    void release()
    {
        _value.reset();
    }

    operator T const&() const { return value(); }

private:
    boost::optional<T const&> _value;
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_VALUEIO_TVALUE_INPUT_HPP