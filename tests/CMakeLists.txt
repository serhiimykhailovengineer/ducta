if (DUCTA_USE_EMBEDDED_BACKEND)
    add_compile_definitions(DUCTA_USE_HOST_PLATFORM)
endif()

add_subdirectory(mocks)
add_subdirectory(unit_tests)
add_subdirectory(int_tests)
if (NOT MSVC)
    add_subdirectory(no_exceptions)
endif()

# Build all test executables and run them through CTest:
#   cmake --build --preset conan-release --target check
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS CoreTests IOTests SteadyClockTests PipelineTests ConfigurationTests UtilsTests LogDisabledTests IntTests
    USES_TERMINAL
)
if (TARGET NoExceptionsApp)
    add_dependencies(check NoExceptionsApp NoExceptionsPanic NoExceptionsPanicHandler
        CapacityMismatchApp)
endif()
