#pragma once

#include <gmock/gmock.h>

#include "ducta/Core/Types/TypeIndex.hpp"

namespace ducta {
namespace IO {


template <typename T>
class TInputMock
{
public:
    using value_type = std::decay_t<T>;
    using storage_type = std::conditional_t<std::is_reference<T>::value, const value_type&, value_type>;

public:
    MOCK_METHOD(bool, ready, (), (const));
    MOCK_METHOD(bool, compatible, (TypeIndex type), (const));
    MOCK_METHOD(void, notify, (value_type const& value), ());
};

template <typename T>
class TInputMockWrapper
{
public:
    using value_type = typename TInputMock<T>::value_type;
    using storage_type = typename TInputMock<T>::storage_type;

public:
    explicit TInputMockWrapper(TInputMock<T>& mock_ref)
        : mock(mock_ref)
    {}

    TInputMock<T>& mock;

    bool ready() const
    {
        return mock.ready();
    }

    bool compatible(TypeIndex type) const
    {
        return mock.compatible(type);
    }

    void notify(value_type const& value)
    {
        mock.notify(value);
    }

};

} // namespace IO
} // namespace ducta