#pragma once

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"

namespace ducta {
namespace Test {

template <class T>
class Adder
{
public:
    IO::TInput<T> left;
    IO::TInput<T> right;
    IO::TOutput<T> result;

public:
    void init() {}

    void iterate()
    {
        if (!left.ready() || !right.ready())
        {
            return;
        }

        result = left.value() + right.value();
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

