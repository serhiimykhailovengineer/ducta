set(TARGET_NAME CoreTests)

add_executable(${TARGET_NAME}
    SourceLocationTest.cpp
    CheckedMathTest.cpp
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ducta::ducta
)

gtest_discover_tests(${TARGET_NAME})
