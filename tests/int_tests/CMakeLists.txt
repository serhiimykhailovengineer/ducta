set(TARGET_NAME IntTests)

add_executable(${TARGET_NAME}
    ValueIOTest.cpp
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
)