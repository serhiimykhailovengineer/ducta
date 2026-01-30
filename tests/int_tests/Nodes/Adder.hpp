#pragma once

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"

#include "ducta/Utils/Expected.hpp"
#include "ducta/Utils/String.hpp"

namespace ducta {
namespace Test {

struct AdderError
{
    String<100> message;

    explicit AdderError()
    {}

    explicit AdderError(String<100> msg)
    : message{msg}
    {}
};

template <class T>
class Adder
{
public:
    IO::TInput<T> left;
    IO::TInput<T> right;
    IO::TOutput<T> result;

public:
    void init() {}

    Utils::Expected<bool, AdderError> iterate()
    {
        if (!left.ready() || !right.ready())
        {
            return Utils::Unexpected<AdderError>{AdderError{"Inputs not ready"}};
        }

        result = left.value() + right.value();
        return true;
    }

    void release() {}
};

} // Test
} // ducta

DEFINE_NODE_INPUTS(ducta::Test::Adder<int>,
    ("left", left),
    ("right", right)
);

DEFINE_NODE_OUTPUTS(ducta::Test::Adder<int>,
    ("result", result)
);

