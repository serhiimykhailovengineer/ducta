if (DUCTA_USE_EMBEDDED_BACKEND)
    add_compile_definitions(DUCTA_USE_HOST_PLATFORM)
endif()

add_subdirectory(mocks)
add_subdirectory(unit_tests)
add_subdirectory(int_tests)