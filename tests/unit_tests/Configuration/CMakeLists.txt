set(TARGET_NAME ConfigurationTests)

add_executable(${TARGET_NAME}
    PropTreeTest.cpp
    DecodeTest.cpp
    VariantTest.cpp
    ContainersTest.cpp
    JsonTest.cpp
    DurationTest.cpp
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ducta::ducta
)

gtest_discover_tests(${TARGET_NAME})
