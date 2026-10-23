cmake_minimum_required(VERSION 3.21)

project(Ducta
    VERSION 0.2.1
    DESCRIPTION "Modular C++ data-flow framework for connecting and executing computational nodes"
    HOMEPAGE_URL "https://github.com/serhiimykhailovengineer/ducta"
    LANGUAGES CXX
)

# ========== Options ==========

# Backend: std (standard library, Boost, fmt) or etl (Embedded Template Library, no heap)
set(DUCTA_BACKEND_DEFAULT "std")
if (DEFINED USE_EMBEDDED_BACKEND AND NOT DEFINED DUCTA_BACKEND) # former option
    message(DEPRECATION "USE_EMBEDDED_BACKEND is replaced by DUCTA_BACKEND=std|etl")
    if (USE_EMBEDDED_BACKEND)
        set(DUCTA_BACKEND_DEFAULT "etl")
    endif()
endif()
set(DUCTA_BACKEND "${DUCTA_BACKEND_DEFAULT}" CACHE STRING
    "Backend: std (standard library, Boost, fmt) or etl (Embedded Template Library)")
set_property(CACHE DUCTA_BACKEND PROPERTY STRINGS std etl)
if (NOT DUCTA_BACKEND MATCHES "^(std|etl)$")
    message(FATAL_ERROR "DUCTA_BACKEND must be 'std' or 'etl', got '${DUCTA_BACKEND}'")
endif()
if (DUCTA_BACKEND STREQUAL "etl")
    set(DUCTA_USE_EMBEDDED_BACKEND TRUE)
else()
    set(DUCTA_USE_EMBEDDED_BACKEND FALSE)
endif()
message(STATUS "Ducta ${PROJECT_VERSION}, backend: ${DUCTA_BACKEND}")

option(DUCTA_BUILD_TESTS "Build the tests of Ducta" ${PROJECT_IS_TOP_LEVEL})
option(DUCTA_INSTALL "Generate the install rules of Ducta" ${PROJECT_IS_TOP_LEVEL})

# Capacities and features (DUCTA_OPTION_DEFINITIONS)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DuctaOptions.cmake)

# ========== Dependencies ==========
# A target defined by the parent project (e.g. with FetchContent) is used as it is

if (DUCTA_USE_EMBEDDED_BACKEND)
    if (NOT TARGET etl::etl)
        find_package(etl 20.46 REQUIRED) # 20.46: formats long (int64_t on 64-bit Linux)
    endif()
    set(DUCTA_DEPENDENCIES etl::etl)
else()
    if (NOT TARGET Boost::headers)
        find_package(Boost REQUIRED)
    endif()
    if (NOT TARGET fmt::fmt)
        find_package(fmt REQUIRED)
    endif()
    set(DUCTA_DEPENDENCIES Boost::headers fmt::fmt)
endif()

# ========== Library (header-only) ==========

include(GNUInstallDirs)

add_library(ducta INTERFACE)
add_library(ducta::ducta ALIAS ducta)

target_include_directories(ducta
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(ducta INTERFACE cxx_std_17)
if (DUCTA_USE_EMBEDDED_BACKEND)
    target_compile_definitions(ducta INTERFACE DUCTA_USE_EMBEDDED_BACKEND)
else()
    target_compile_definitions(ducta INTERFACE DUCTA_USE_STD_BACKEND)
endif()
target_compile_definitions(ducta INTERFACE ${DUCTA_OPTION_DEFINITIONS})
target_link_libraries(ducta INTERFACE ${DUCTA_DEPENDENCIES})

# ========== Install: find_package(ducta) ==========

if (DUCTA_INSTALL)
    include(CMakePackageConfigHelpers)

    set(DUCTA_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/cmake/ducta)

    install(TARGETS ducta EXPORT ductaTargets)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/ducta)
    install(EXPORT ductaTargets
        NAMESPACE ducta::
        DESTINATION ${DUCTA_CMAKE_INSTALL_DIR}
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ductaConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ductaConfig.cmake
        INSTALL_DESTINATION ${DUCTA_CMAKE_INSTALL_DIR}
    )
    # 0.x: a new minor version may break the API
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ductaConfigVersion.cmake
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ductaConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ductaConfigVersion.cmake
        DESTINATION ${DUCTA_CMAKE_INSTALL_DIR}
    )
endif()

# ========== Tests ==========

if (DUCTA_BUILD_TESTS)
    # The version is set here; library.json (PlatformIO) must follow it (conanfile.py reads it)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/library.json DUCTA_LIBRARY_JSON)
    string(JSON DUCTA_LIBRARY_JSON_VERSION GET "${DUCTA_LIBRARY_JSON}" version)
    if (NOT DUCTA_LIBRARY_JSON_VERSION STREQUAL PROJECT_VERSION)
        message(FATAL_ERROR
            "library.json has version ${DUCTA_LIBRARY_JSON_VERSION}, CMakeLists.txt ${PROJECT_VERSION}")
    endif()

    find_package(GTest REQUIRED)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()
