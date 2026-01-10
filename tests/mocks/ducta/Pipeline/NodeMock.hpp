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

class IterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, iterate, (), ());
    MOCK_METHOD(void, release, (), ());
};

class IterableNodeMockWrapper
{
public:
    std::reference_wrapper<IterableNodeMock> mock;

    void init() 
    {
        mock.get().init();
    }

    void iterate() 
    {
        mock.get().iterate();
    }

    void release() 
    {
        mock.get().release();
    }
};



} // namespace Pipeline
} // namespace ducta