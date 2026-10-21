# The library must compile and work without exceptions (-fno-exceptions), as most firmware is built

function(ducta_no_exceptions_executable TARGET_NAME SOURCE)
    add_executable(${TARGET_NAME} ${SOURCE})
    target_compile_options(${TARGET_NAME} PRIVATE -fno-exceptions)
    target_link_libraries(${TARGET_NAME} PRIVATE ducta::ducta)
endfunction()

# The engine, from the JSON configuration to run()
ducta_no_exceptions_executable(NoExceptionsApp NoExceptionsApp.cpp)
add_test(NAME NoExceptionsApp COMMAND NoExceptionsApp)

# ducta::fatal_error aborts, without a handler and after a handler that returns
ducta_no_exceptions_executable(NoExceptionsPanic PanicApp.cpp)
ducta_no_exceptions_executable(NoExceptionsPanicHandler PanicApp.cpp)
target_compile_definitions(NoExceptionsPanicHandler PRIVATE DUCTA_PANIC_HANDLER=test_panic_handler)

# Source files of one program with different capacities: the check panics before main()
ducta_no_exceptions_executable(CapacityMismatchApp CapacityMismatchApp.cpp)
target_sources(CapacityMismatchApp PRIVATE CapacityMismatchOther.cpp)
target_compile_definitions(CapacityMismatchApp PRIVATE DUCTA_PANIC_HANDLER=test_panic_handler)
set_source_files_properties(CapacityMismatchOther.cpp PROPERTIES COMPILE_DEFINITIONS DUCTA_MAX_NODES=7)
add_test(NAME CapacityMismatch
    COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:CapacityMismatchApp> -DMODE=none
        "-DEXPECTED=panic handler: Ducta capacities or backend differ between source files"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckPanic.cmake)

foreach(mode IN ITEMS input notify)
    if (mode STREQUAL "input")
        set(started "reading an input that is not ready")
        set(message "Input not ready")
    else()
        set(started "notifying an int input with a double")
        set(message "Incompatible type for notification")
    endif()
    add_test(NAME NoExceptionsPanic.${mode}
        COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:NoExceptionsPanic> -DMODE=${mode}
            "-DEXPECTED=${started}" "-DUNEXPECTED=panic handler"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckPanic.cmake)
    add_test(NAME NoExceptionsPanicHandler.${mode}
        COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:NoExceptionsPanicHandler> -DMODE=${mode}
            "-DEXPECTED=panic handler: ${message}" -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckPanic.cmake)
endforeach()
