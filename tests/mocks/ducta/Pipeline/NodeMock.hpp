#pragma once

#include <gmock/gmock.h>

#include "ducta/Core/Types/ReferenceWrapper.hpp"

namespace ducta {
namespace Pipeline {

class NonIterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, release, (), ());
};

template <class IterateResult = void>
class IterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(IterateResult, iterate, (), ());
    MOCK_METHOD(void, release, (), ());
};



} // namespace Pipeline
} // namespace ducta