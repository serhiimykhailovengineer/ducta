#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

#include "ducta/io/Private/TOutputModel.hpp"

namespace ducta {
namespace IO {

template <typename T>
class TOutput
{
public:
    template <typename TOutputType>
    TOutput(TOutputType&& output);

    template <typename TOutputType, typename... Args>
    TOutput(std::in_place_type_t<TOutputType>, Args&&... args);

    void set(T const& value);
    void set(T&& value);

    TOutput<T>& operator=(T const& value);

    TOutput<T>& operator=(T&& value);

    OutputRef get_ref();

private:
    std::unique_ptr<Private::TOutputConcept<T>> m_output;
};

template <typename T>
template <typename TOutputType>
TOutput<T>::TOutput(TOutputType&& output)
: m_output(std::make_unique<Private::TOutputModel<std::decay_t<TOutputType>, T>>(std::forward<TOutputType>(output)))
{
}

template <typename T>
template <typename TOutputType, typename... Args>
TOutput<T>::TOutput(std::in_place_type_t<TOutputType>, Args&&... args)
: m_output(std::make_unique<Private::TOutputModel<std::decay_t<TOutputType>, T>>(std::in_place_t{}, std::forward<Args>(args)...))
{
}

template <typename T>
void TOutput<T>::set(T const& value) 
{
    m_output->set(value);
}

template <typename T>
void TOutput<T>::set(T&& value)
{
    m_output->set(std::move(value));
}

template <typename T>
TOutput<T>& TOutput<T>::operator=(T const& value)
{
    set(value);
    return *this;
}

template <typename T>
TOutput<T>& TOutput<T>::operator=(T&& value)
{
    set(std::move(value));
    return *this;
}

template <typename T>
OutputRef TOutput<T>::get_ref()
{
    return m_output->get_ref();
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_HPP