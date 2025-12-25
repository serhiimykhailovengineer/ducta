#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

#include "ducta/io/TInput.hpp"
#include "ducta/io/InputRef.hpp"

#include "ducta/Utils/TypeIndex.hpp"
#include "ducta/Utils/Deferred.hpp"

namespace ducta {
namespace IO {

template <typename T>
class TOutput
{
public:
    using value_type = T;

public:
    TOutput()
    : _value{}
    {
    }

    Utils::TypeIndex type_id() const
    {
        return Utils::type_id<T>();
    }

    void set(T const& value) 
    {
        _value = value;
        for (auto& input_ref : _inputs)
        {
            input_ref.notify(_value);
        }
    }

    void set(T&& value) 
    {
        _value = std::move(value);
        for (auto& input_ref : _inputs)
        {
            input_ref.notify(_value);
        }
    }

    TOutput<T>& operator=(T const& value)
    {
        set(value);
        return *this;
    }

    TOutput<T>& operator=(T&& value)
    {
        set(std::move(value));
        return *this;
    }

    Utils::Deferred bind(InputRef input)
    {
        if (input.compatible(Utils::type_id<T>()))
        {
            _inputs.emplace_back(input);
            return Utils::Deferred{[this, &input]() {
                // Remove input from the list upon destruction
                _inputs.erase(std::remove_if(_inputs.begin(), _inputs.end(),
                    [&input](InputRef& ref) { return ref == input; }),
                    _inputs.end());
            }};
        }

        return {};
    }

private:
    T _value;
    std::vector<InputRef> _inputs;
};

template <typename T>
Utils::Deferred bind(TOutput<T>& output, InputRef input)
{
    return output.bind(input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_HPP