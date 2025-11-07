cmake_minimum_required(VERSION 3.30)

project(Ducta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
)

# Завантажує і додає в проект
FetchContent_MakeAvailable(googletest)

add_library(ducta_io INTERFACE)
target_include_directories(ducta_io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


add_subdirectory(tests)