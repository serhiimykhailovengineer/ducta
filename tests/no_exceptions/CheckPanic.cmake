# Runs PROGRAM with MODE and checks that it was aborted (SIGABRT) after printing EXPECTED, and
# that it never printed "panic returned" (nor UNEXPECTED, if given).
#   cmake -DPROGRAM=<exe> -DMODE=<input|notify> -DEXPECTED=<text> [-DUNEXPECTED=<text>]
#         -P CheckPanic.cmake
execute_process(
    COMMAND "${PROGRAM}" "${MODE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
)
message(STATUS "exit: ${result}\n${output}")

if ("${result}" MATCHES "^[0-9]+$")
    message(FATAL_ERROR "expected the program to abort, it exited with ${result}")
endif()
if (NOT "${result}" MATCHES "[Aa]bort")
    message(FATAL_ERROR "expected an abort, got '${result}'")
endif()
string(FIND "${output}" "${EXPECTED}" found)
if (found EQUAL -1)
    message(FATAL_ERROR "expected the output to contain '${EXPECTED}'")
endif()
string(FIND "${output}" "panic returned" returned)
if (NOT returned EQUAL -1)
    message(FATAL_ERROR "the program continued after the panic")
endif()
if (DEFINED UNEXPECTED)
    string(FIND "${output}" "${UNEXPECTED}" unexpected)
    if (NOT unexpected EQUAL -1)
        message(FATAL_ERROR "the output must not contain '${UNEXPECTED}'")
    endif()
endif()
