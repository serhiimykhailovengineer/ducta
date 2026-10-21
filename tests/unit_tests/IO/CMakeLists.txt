set(TARGET_NAME IOTests)

add_executable(${TARGET_NAME}
    DefineInputsTest.cpp
    TInputTest.cpp
    InputRefTest.cpp
    TOutputTest.cpp
    OutputRefTest.cpp
    TEventTest.cpp
    EventRefTest.cpp
    DefineEventsTest.cpp
    EventHandlerTest.cpp
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ducta::ducta
        ducta_io_mocks
)

gtest_discover_tests(${TARGET_NAME})
