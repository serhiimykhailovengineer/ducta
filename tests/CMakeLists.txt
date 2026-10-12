if (DUCTA_USE_EMBEDDED_BACKEND)
    add_compile_definitions(DUCTA_USE_HOST_PLATFORM)
endif()

add_subdirectory(mocks)
add_subdirectory(unit_tests)
add_subdirectory(int_tests)

# Build all test executables and run them through CTest:
#   cmake --build --preset conan-release --target check
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS IOTests SteadyClockTests PipelineTests IntTests
    USES_TERMINAL
)
