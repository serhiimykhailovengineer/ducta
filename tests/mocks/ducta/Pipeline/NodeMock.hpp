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


class NonIterableNodeMockWrapper
{
public:
    ::ducta::reference_wrapper<NonIterableNodeMock> mock;

    explicit NonIterableNodeMockWrapper(NonIterableNodeMock& mock)
        : mock(::ducta::ref(mock))
    {}

    void init() 
    {
        mock.get().init();
    }

    void release() 
    {
        mock.get().release();
    }
};

template <class IterateResult = void>
class IterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(IterateResult, iterate, (), ());
    MOCK_METHOD(void, release, (), ());
};

template <class IterateResult = void>
class IterableNodeMockWrapper
{
public:
    using MockType = IterableNodeMock<IterateResult>;

    explicit IterableNodeMockWrapper(MockType& m) noexcept
        : mock(m)
    {
    }
    
    
    void init() 
    {
        mock.get().init();
    }

    template <class R = IterateResult,
              std::enable_if_t<!std::is_void_v<R>, int> = 0>
    R iterate()
    {
        return mock.get().iterate();
    }

    template <class R = IterateResult,
              std::enable_if_t<std::is_void_v<R>, int> = 0>
    void iterate()
    {
        mock.get().iterate();
    }

    void release() 
    {
        mock.get().release();
    }

public:
    ::ducta::reference_wrapper<MockType> mock;
};



} // namespace Pipeline
} // namespace ducta