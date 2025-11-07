#pragma once

#include <gmock/gmock.h>

namespace ducta {
namespace IO {

template <typename T>
class TInputMock
{
public:
    MOCK_METHOD(bool, ready, (), (const));
    MOCK_METHOD(T const&, value, (), (const));
    MOCK_METHOD(void, notify, (T const& value), ());
    MOCK_METHOD(void, set_callback, (std::function<void(T const&)>), ());
};

template <typename T>
class TInputMockWrapper
{
public:
    TInputMock<T>& mock;

    bool ready() const
    {
        return mock.ready();
    }

    T const& value() const
    {
        return mock.value();
    }

    void notify(T const& value)
    {
        mock.notify(value);
    }

    void set_callback(std::function<void(T const&)> callback)
    {
        mock.set_callback(std::move(callback));
    }

};

} // namespace IO
} // namespace ducta