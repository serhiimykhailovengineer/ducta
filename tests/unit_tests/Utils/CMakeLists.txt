set(TARGET_NAME UtilsTests)

add_executable(${TARGET_NAME}
    LogTest.cpp
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ducta::ducta
)

gtest_discover_tests(${TARGET_NAME})

# The build without logging (DUCTA_ENABLE_LOGGING=0): a program of its own, as the whole program
# must use the same setting
add_executable(LogDisabledTests LogDisabledTest.cpp)
target_compile_definitions(LogDisabledTests PRIVATE DUCTA_ENABLE_LOGGING=0)
if (NOT MSVC)
    target_compile_options(LogDisabledTests PRIVATE -Wall -Wextra -Werror=unused)
endif()
target_link_libraries(LogDisabledTests
    PUBLIC
        GTest::Main
        GTest::gtest
        ducta::ducta
)
gtest_discover_tests(LogDisabledTests)
