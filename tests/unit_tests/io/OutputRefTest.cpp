#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/InputRef.hpp"

#include "ducta/io/TOutput.hpp"
#include "ducta/io/OutputRef.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(OutputRefTests, value_output) 
{
    IO::TInput<int> input{};

    IO::InputRef input_ref{input};
    
    IO::TOutput<int> output{};
    IO::OutputRef output_ref{output};
    
    auto deferred = bind(output_ref, input_ref);

}