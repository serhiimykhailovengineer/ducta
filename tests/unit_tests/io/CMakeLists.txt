set(TARGET_NAME IOTests)

add_executable(${TARGET_NAME}
    TInputTest.cpp
    TOutputTest.cpp
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        gtest_main
        gtest
        gmock
        ducta_io
        ducta_io_mocks
)