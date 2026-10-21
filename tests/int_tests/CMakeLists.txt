set(TARGET_NAME IntTests)

add_executable(${TARGET_NAME}
    IOSizeTest.cpp
    ValueIOTest.cpp
    PipelineTest.cpp
    DuctaEngineTest.cpp
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
)

gtest_discover_tests(${TARGET_NAME})
