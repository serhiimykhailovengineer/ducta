// Built with -fno-exceptions: misuse calls ducta::panic, which must call DUCTA_PANIC_HANDLER (when
// the application defines it) and then abort, even if the handler returns.
// Run by CheckPanic.cmake: PanicApp <input|notify>

#if defined(__cpp_exceptions)
#error "this test must be compiled with -fno-exceptions"
#endif

#include <cstdio>
#include <cstring>

#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/TInput.hpp"

#if defined(DUCTA_PANIC_HANDLER)
#include "ducta/Core/Panic.hpp"

/* Returns: ducta::panic must abort afterwards */
void DUCTA_PANIC_HANDLER(char const* message)
{
    std::fprintf(stderr, "panic handler: %s\n", message);
}
#endif

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <boost/assert/source_location.hpp>
#include <exception>

#include "ducta/Core/Panic.hpp"

/* Boost without exceptions (BOOST_NO_EXCEPTIONS) calls these: the application defines them */
namespace boost {
[[noreturn]] void throw_exception(std::exception const& e)
{
    ::ducta::panic(e.what());
}

[[noreturn]] void throw_exception(std::exception const& e, boost::source_location const&)
{
    ::ducta::panic(e.what());
}
} // namespace boost
#endif

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: %s <input|notify>\n", argv[0]);
        return 2;
    }

    ducta::IO::TInput<int> input;
    if (std::strcmp(argv[1], "input") == 0)
    {
        std::fprintf(stderr, "reading an input that is not ready\n");
        int const value = input.value(); // panics: "Input not ready"
        std::fprintf(stderr, "panic returned (%d)\n", value);
    }
    else if (std::strcmp(argv[1], "notify") == 0)
    {
        std::fprintf(stderr, "notifying an int input with a double\n");
        ducta::IO::InputRef{input}.notify(1.5); // panics: "Incompatible type for notification"
        std::fprintf(stderr, "panic returned\n");
    }
    else
    {
        std::fprintf(stderr, "unknown mode '%s'\n", argv[1]);
        return 2;
    }
    return 0;
}
