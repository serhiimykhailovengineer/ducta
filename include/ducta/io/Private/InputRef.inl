#ifndef DUCTA_INPUT_REF_INL
#define DUCTA_INPUT_REF_INL

#include "ducta/io/Private/InputModelRef.hpp"

namespace ducta {
namespace IO {


template <typename TInputType>
InputRef::InputRef(TInputType&& input)
: m_input(std::make_unique<Private::InputModelRef<std::decay_t<TInputType>>>(std::forward<TInputType>(input)))
{
}

bool InputRef::ready() const
{
    return m_input->is_ready();
}

} // namespace io
} // namespace ducta

#endif // DUCTA_INPUT_REF_INL