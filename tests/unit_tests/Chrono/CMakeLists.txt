set(TARGET_NAME SteadyClockTests)

add_executable(${TARGET_NAME}
    SteadyClockTests.cpp
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ducta::ducta
)

gtest_discover_tests(${TARGET_NAME})
