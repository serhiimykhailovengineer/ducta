cmake_minimum_required(VERSION 3.20)

project(Ducta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

add_library(ducta_io INTERFACE)
target_include_directories(ducta_io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(src)
add_subdirectory(tests)