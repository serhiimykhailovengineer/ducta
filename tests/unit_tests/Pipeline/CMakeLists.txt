set(TARGET_NAME PipelineTests)

add_executable(${TARGET_NAME}
    NodeTests.cpp
    PipelineTest.cpp
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

set(BACKEND_LIBS)

if (DUCTA_USE_EMBEDDED_BACKEND)
    list(APPEND BACKEND_LIBS etl::etl)
else()
    list(APPEND BACKEND_LIBS Boost::boost fmt::fmt)
endif()

target_link_libraries(${TARGET_NAME}
    PUBLIC
        GTest::Main
        GTest::gtest
        GTest::gmock
        ${BACKEND_LIBS}
        ducta_io
        ducta_io_mocks
        ducta_pipeline
)