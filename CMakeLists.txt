cmake_minimum_required(VERSION 3.30)

project(Ducta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)

add_library(ducta_io INTERFACE)
target_include_directories(ducta_io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


add_subdirectory(tests)