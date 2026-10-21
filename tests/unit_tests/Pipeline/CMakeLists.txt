set(TARGET_NAME PipelineTests)

add_executable(${TARGET_NAME}
    NodeTests.cpp
    PipelineTest.cpp
    EventWiringTest.cpp
    NodeFactoryTest.cpp
    NodeManagerTest.cpp
    DuctaEngineTest.cpp
    ConnectionsTest.cpp
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
