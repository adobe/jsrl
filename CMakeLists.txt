cmake_minimum_required(VERSION 3.14...3.27)

project(jsrl
    VERSION 1.0.0
    DESCRIPTION "JSON Serialization and Reading Library - A modern C++ JSON library"
    LANGUAGES CXX
)

# Only set the cxx_standard if it's not set by someone else
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Provide option to build as shared library
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Option to build tests
option(JSRL_BUILD_TESTS "Build JSRL tests" ON)

# Option to install the library
option(JSRL_INSTALL "Generate install target" ON)

# Create library target
add_library(jsrl
    src/jsrl.cpp
    src/jsrl.hpp
    src/jsrl_format.hpp
    src/jsrl_general_number.cpp
    src/jsrl_general_number.hpp
    src/jsrl_impl_util.cpp
    src/jsrl_impl_util.hpp
    src/jsrl_mod.hpp
    src/jsrlpp.cpp
    src/jsrlpp.hpp
)

# Add an alias so that library can be used inside the build tree
add_library(jsrl::jsrl ALIAS jsrl)

# Set target properties
set_target_properties(jsrl PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Generate export header for symbol visibility
include(GenerateExportHeader)
generate_export_header(jsrl
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/jsrl_export.hpp
)

# Static library consumers must define JSRL_STATIC_DEFINE so the export
# header suppresses __declspec(dllimport) / visibility("default") attributes.
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(jsrl PUBLIC JSRL_STATIC_DEFINE)
endif()

# Define headers for this library
target_include_directories(jsrl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Require C++17
target_compile_features(jsrl PUBLIC cxx_std_17)

# Add compiler warnings
if(MSVC)
    target_compile_options(jsrl PRIVATE /W4)
else()
    target_compile_options(jsrl PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules
if(JSRL_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install library
    install(TARGETS jsrl
        EXPORT jsrlTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install headers
    install(FILES
        src/jsrl.hpp
        src/jsrl_format.hpp
        src/jsrl_general_number.hpp
        src/jsrl_mod.hpp
        src/jsrlpp.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/jsrl_export.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jsrl
    )

    # Create and install package config files
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/jsrlConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jsrlConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/jsrlConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jsrl
    )

    install(EXPORT jsrlTargets
        FILE jsrlTargets.cmake
        NAMESPACE jsrl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jsrl
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/jsrlConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/jsrlConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jsrl
    )
endif()

# Testing
if(JSRL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
