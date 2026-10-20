# The library must compile and work without exceptions (-fno-exceptions), as most firmware is built
set(TARGET_NAME NoExceptionsApp)

add_executable(${TARGET_NAME} NoExceptionsApp.cpp)
target_compile_options(${TARGET_NAME} PRIVATE -fno-exceptions)
if (NOT DUCTA_USE_EMBEDDED_BACKEND)
    target_link_libraries(${TARGET_NAME} PRIVATE Boost::boost fmt::fmt)
endif()
target_link_libraries(${TARGET_NAME} PRIVATE ducta_pipeline)

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
