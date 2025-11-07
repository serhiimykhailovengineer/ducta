#ifndef DUCTA_T_INPUT_CONCEPT_HPP
#define DUCTA_T_INPUT_CONCEPT_HPP

#include "ducta/io/InputRef.hpp"
#include "ducta/io/Private/InputConcept.hpp"

#include <functional>

namespace ducta {
namespace IO {
namespace Private {

template <typename ValueType>
class TInputConcept : public InputConcept
{
public:
    virtual ~TInputConcept() = default;

    virtual ValueType const& get_value() const = 0;
    virtual void do_notify(ValueType const& value) = 0;
    virtual void set_callback(std::function<void(ValueType const&)> callback) = 0;
    // virtual InputRef get_ref() = 0;
};

   
} // namespace Private
} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_CONCEPT_HPP