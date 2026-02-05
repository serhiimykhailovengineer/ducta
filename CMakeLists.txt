cmake_minimum_required(VERSION 3.20)

project(Ducta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

option(USE_EMBEDDED_BACKEND "Use embedded backend" OFF)

message(STATUS "Use embedded backend ${USE_EMBEDDED_BACKEND}")

if (USE_EMBEDDED_BACKEND)
    set(DUCTA_USE_EMBEDDED_BACKEND TRUE)
    add_definitions(-DDUCTA_USE_EMBEDDED_BACKEND)
    find_package(etl REQUIRED)
else()
    set(DUCTA_USE_EMBEDDED_BACKEND FALSE)
    find_package(Boost REQUIRED COMPONENTS system)
    find_package(fmt REQUIRED)
endif()

find_package(GTest REQUIRED)

add_library(ducta_io INTERFACE)
target_include_directories(ducta_io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(ducta_pipeline INTERFACE)
target_include_directories(ducta_pipeline
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(ducta_pipeline
    INTERFACE
        ducta_io
)

add_subdirectory(tests)