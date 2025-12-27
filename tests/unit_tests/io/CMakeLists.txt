set(TARGET_NAME IOTests)

add_executable(${TARGET_NAME}
    TInputTest.cpp
    InputRefTest.cpp
    TOutputTest.cpp
    OutputRefTest.cpp
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
        Boost::boost
        ducta_io
        ducta_io_mocks
)