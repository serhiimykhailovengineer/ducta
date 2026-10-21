import os
import re

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conan.tools.files import copy, load

required_conan_version = ">=2.0"


class DuctaConan(ConanFile):
    """
    Ducta as a Conan package (header-only):

        conan create . -o "ducta/*:backend=etl"
        # conanfile of the application
        requires = "ducta/0.1.0"
        default_options = {"ducta/*:backend": "etl", "ducta/*:capacity_profile": "mcu"}

    Capacities are configured by the application in ducta_config.hpp (see
    include/ducta/Core/Config.hpp), not with options: they do not change the package.

    Development of Ducta itself (all backends and the tests), as project/configure.sh does:

        conan install . --output-folder=build/conan -o "&:develop=True"
    """

    name = "ducta"
    description = "Modular C++ data-flow framework for connecting and executing computational nodes"
    license = "MIT"
    url = "https://github.com/serhiimykhailovengineer/ducta"
    homepage = "https://github.com/serhiimykhailovengineer/ducta"
    topics = ("dataflow", "pipeline", "embedded", "robotics", "header-only")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        # std: standard library, Boost, fmt; etl: Embedded Template Library, no heap
        "backend": ["std", "etl"],
        # mcu: small capacities and no execution trace (DUCTA_CAPACITY_PROFILE_MCU)
        "capacity_profile": ["default", "mcu"],
        # None: the default of the capacity profile
        "tracing": [None, True, False],
        # False: the DUCTA_LOG_* calls compile to nothing (DUCTA_ENABLE_LOGGING 0)
        "logging": [True, False],
        # Dependencies of every backend and of the tests, to develop Ducta
        "develop": [True, False],
    }
    default_options = {
        "backend": "std",
        "capacity_profile": "default",
        "tracing": None,
        "logging": True,
        "develop": False,
        # Ducta uses only header-only parts of Boost
        "boost/*:header_only": True,
    }
    exports_sources = "include/*", "LICENSE"
    no_copy_source = True

    def set_version(self):
        if self.version:
            return
        cmake = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
        self.version = re.search(r"project\(Ducta\s+VERSION\s+([0-9.]+)", cmake).group(1)

    def requirements(self):
        if self.options.develop or self.options.backend == "std":
            self.requires("boost/1.89.0", transitive_headers=True)
            self.requires("fmt/12.1.0", transitive_headers=True, transitive_libs=True)
        if self.options.develop or self.options.backend == "etl":
            self.requires("etl/20.49.0", transitive_headers=True)

    def build_requirements(self):
        if self.options.develop:
            self.test_requires("gtest/1.17.0")

    def validate(self):
        check_min_cppstd(self, 17)

    def generate(self):
        # Used by the CMake presets of Ducta (develop=True)
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()

    def package(self):
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(
            self,
            "*",
            os.path.join(self.source_folder, "include"),
            os.path.join(self.package_folder, "include"),
        )

    def package_id(self):
        # The same headers for every option: the options are compile definitions of the consumer
        self.info.clear()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "ducta")
        self.cpp_info.set_property("cmake_target_name", "ducta::ducta")
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

        if self.options.backend == "etl":
            self.cpp_info.defines.append("DUCTA_USE_EMBEDDED_BACKEND")
        else:
            self.cpp_info.defines.append("DUCTA_USE_STD_BACKEND")
        if self.options.capacity_profile == "mcu":
            self.cpp_info.defines.append("DUCTA_CAPACITY_PROFILE_MCU")
        if self.options.tracing is not None and str(self.options.tracing) != "None":
            self.cpp_info.defines.append(
                "DUCTA_ENABLE_TRACING=1" if self.options.tracing else "DUCTA_ENABLE_TRACING=0"
            )
        if not self.options.logging:
            self.cpp_info.defines.append("DUCTA_ENABLE_LOGGING=0")
