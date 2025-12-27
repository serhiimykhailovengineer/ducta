#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/InputRef.hpp"

#include "ducta/Utils/TypeIndex.hpp"
#include "ducta/Utils/Deferred.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents an output of a specific type. It can bind to inputs of compatible type. Write-only.
 * @tparam T The type of value this output handles. Can be a value type.
 * 
 * This class provides methods to check compatibility with types, set values, and notify inputs of new values.
 */
template <typename T>
class TOutput
{
public:
    using value_type = std::decay_t<T>;
    using storage_type = value_type;

public:
    TOutput()
    : _value{}
    {
    }

    Utils::TypeIndex type_id() const
    {
        return Utils::type_id<T>();
    }

    void set(value_type const& value) 
    {
        _value = value;
        for (auto& input_ref : _inputs)
        {
            input_ref.notify(_value);
        }
    }

    void set(value_type&& value) 
    {
        _value = std::move(value);
        for (auto& input_ref : _inputs)
        {
            input_ref.notify(_value);
        }
    }

    TOutput<T>& operator=(value_type const& value)
    {
        set(value);
        return *this;
    }

    TOutput<T>& operator=(value_type&& value)
    {
        set(std::move(value));
        return *this;
    }

    Utils::Deferred bind(InputRef const& input)
    {
        if (input.compatible(Utils::type_id<T>()))
        {
            _inputs.emplace_back(input);
            return Utils::Deferred{[this, input = std::move(input)]() {
                // Remove input from the list upon destruction
                _inputs.erase(std::remove_if(_inputs.begin(), _inputs.end(),
                    [&input](InputRef& ref) { return ref == input; }),
                    _inputs.end());
            }};
        }

        return {};
    }

private:
    value_type _value;
    std::vector<InputRef> _inputs;
};

template <typename T>
Utils::Deferred bind(TOutput<T>& output, InputRef const& input)
{
    return output.bind(input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_HPP