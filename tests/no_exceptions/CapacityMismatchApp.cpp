// Two source files with different capacities (CapacityMismatchOther.cpp is compiled with another
// DUCTA_MAX_NODES): the check of Capacity.hpp must panic before main().
// Run by CheckPanic.cmake.

#include <cstdio>

#include "ducta/Pipeline/Nodes.hpp"

void test_panic_handler(char const* message)
{
    std::fprintf(stderr, "panic handler: %s\n", message);
}

int other_file_nodes();

int main()
{
    std::fprintf(stderr, "panic returned: main() was reached (%d)\n", other_file_nodes());
    return 0;
}
