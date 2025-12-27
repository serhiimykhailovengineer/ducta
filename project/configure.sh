#!/bin/bash

BUILD_FOLDER=build
BUILD_TYPE=Release

conan install . --output-folder=$BUILD_FOLDER/$BUILD_TYPE --build=missing
cmake -S . -B $BUILD_FOLDER/$BUILD_TYPE -DCMAKE_TOOLCHAIN_FILE=$BUILD_FOLDER/$BUILD_TYPE/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE