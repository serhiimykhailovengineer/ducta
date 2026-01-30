cmake_minimum_required(VERSION 3.20)

project(Ducta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

option(DUCTA_USE_EMBEDDED_BACKEND "Use embedded backend" OFF)

message(STATUS "Use embedded backend ${DUCTA_USE_EMBEDDED_BACKEND}")

if (DUCTA_USE_EMBEDDED_BACKEND)
    add_definitions(-DDUCTA_USE_EMBEDDED_BACKEND)
    find_package(etl REQUIRED)
else()
    find_package(Boost REQUIRED COMPONENTS system)
    find_package(fmt REQUIRED)
endif()

find_package(GTest REQUIRED)

add_library(ducta_io INTERFACE)
target_include_directories(ducta_io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(src)
add_subdirectory(tests)