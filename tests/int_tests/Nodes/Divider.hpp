#pragma once

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"

namespace ducta {
namespace Test {

template <class T>
class Divider {
public:
    IO::TInput<T> left;
    IO::TInput<T> denominator;
    IO::TOutput<T> result;

public:
    void init()
    {
    }

    void iterate()
    {
        if (!left.ready() || !denominator.ready())
        {
            return;
        }

        const T denom = denominator.value();

        if constexpr (std::is_floating_point_v<T>)
        {
            const T eps = std::numeric_limits<T>::epsilon() * T{16};
            if (std::abs(denom) <= eps)
            {
                return; // TODO: Add error processing to iterate
            }
        }
        else
        {
            if (denom == T{})
            {
                return; // TODO: Add error processing to iterate
            }
        }

        result = left.value() / denom;
    }

    void release()
    {
    }
};

} // Test
} // ducta

DEFINE_NODE_INPUTS(ducta::Test::Divider<int>,
    ("left", left),
    ("denominator", denominator)
);

DEFINE_NODE_OUTPUTS(ducta::Test::Divider<int>,
    ("result", result)
);
