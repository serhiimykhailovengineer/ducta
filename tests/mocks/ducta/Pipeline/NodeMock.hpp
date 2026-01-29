#pragma once

#include <gmock/gmock.h>

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
    std::reference_wrapper<NonIterableNodeMock> mock;

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
    std::reference_wrapper<IterableNodeMock<IterateResult>> mock;
    void init() 
    {
        mock.get().init();
    }

    IterateResult iterate() 
    {
        return mock.get().iterate();
    }

    void release() 
    {
        mock.get().release();
    }
};



} // namespace Pipeline
} // namespace ducta