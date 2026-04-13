set(TARGET_NAME SteadyClockTests)

add_executable(${TARGET_NAME}
    SteadyClockTests.cpp
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
        ducta_pipeline
)