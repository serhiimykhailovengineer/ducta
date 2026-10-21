# Consumer of an installed Ducta: used by `conan create` and to check `cmake --install`
cmake_minimum_required(VERSION 3.21)
project(DuctaTestPackage LANGUAGES CXX)

find_package(ducta CONFIG REQUIRED)

add_executable(test_package test_package.cpp)
target_link_libraries(test_package PRIVATE ducta::ducta)
