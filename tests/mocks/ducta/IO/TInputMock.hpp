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

} // namespace IO
} // namespace ducta