cmake_minimum_required(VERSION 3.16)
project(QuestAdbLib VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable utility targets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test programs" OFF)

# Find required packages
find_package(Threads REQUIRED)

# Create the library
add_library(QuestAdbLib
    src/QuestAdbLib.cpp
    src/AdbProcess.cpp
    src/AdbDevice.cpp
    src/AdbCommand.cpp
    src/Utils.cpp
)

# Set library properties
set_target_properties(QuestAdbLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/QuestAdbLib/QuestAdbLib.h;include/QuestAdbLib/AdbDevice.h;include/QuestAdbLib/AdbCommand.h;include/QuestAdbLib/Types.h"
)

# Include directories
target_include_directories(QuestAdbLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(QuestAdbLib
    PRIVATE
        Threads::Threads
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(QuestAdbLib PRIVATE WIN32_LEAN_AND_MEAN QUESTADBLIB_EXPORTS)
    target_link_libraries(QuestAdbLib PRIVATE ws2_32)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(QuestAdbLib PRIVATE LINUX)
endif()

if(APPLE)
    target_compile_definitions(QuestAdbLib PRIVATE MACOS)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(QuestAdbLib PRIVATE /W4)
else()
    target_compile_options(QuestAdbLib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install configuration
include(GNUInstallDirs)

install(TARGETS QuestAdbLib
    EXPORT QuestAdbLibTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QuestAdbLib
)

install(EXPORT QuestAdbLibTargets
    FILE QuestAdbLibTargets.cmake
    NAMESPACE QuestAdbLib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuestAdbLib
)

# Create and install package config
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QuestAdbLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QuestAdbLibConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuestAdbLib
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QuestAdbLibConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QuestAdbLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QuestAdbLibConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuestAdbLib
)

# Export from build tree
export(EXPORT QuestAdbLibTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/QuestAdbLibTargets.cmake"
    NAMESPACE QuestAdbLib::
)

# Note: platform-tools are no longer bundled due to Google's licensing restrictions
# Users must install Android SDK or platform-tools separately