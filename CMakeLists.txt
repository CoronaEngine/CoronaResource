cmake_minimum_required(VERSION 4.0)

# Project definition
project(VisionScene
    VERSION 0.5.0
    DESCRIPTION "VisionScene: Multi-format 3D scene parsing and import library"
    LANGUAGES CXX
)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(VISIONSCENE_BUILD_EXAMPLES "Build VisionScene examples" ON)
option(VISIONSCENE_INSTALL "Enable install rules" ON)

# Allow user to choose static/shared via standard BUILD_SHARED_LIBS
# Default to static library if not specified
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencies
include(FetchContent)

# Fetch ktm math library
FetchContent_Declare(
    ktm
    GIT_REPOSITORY https://github.com/YGXXD/ktm.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ktm)

# Add subdirectories
add_subdirectory(src)

if(VISIONSCENE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation rules
# Note: Headers are handled in src/CMakeLists.txt
if(VISIONSCENE_INSTALL)
    include(CMakePackageConfigHelpers)
    
    set(config_install_dir lib/cmake/VisionScene)
    set(VISIONSCENE_EXPORT_NAME VisionSceneTargets)

    # Install export targets
    install(
        EXPORT ${VISIONSCENE_EXPORT_NAME}
        NAMESPACE VisionScene::
        DESTINATION ${config_install_dir}
    )

    # Generate and install config files
    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/VisionSceneConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
        INSTALL_DESTINATION ${config_install_dir}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        DESTINATION ${config_install_dir}
    )
endif()

# Configuration summary
message(STATUS "")
message(STATUS "=== VisionScene Configuration ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libs:    ${BUILD_SHARED_LIBS}")
message(STATUS "  Build examples: ${VISIONSCENE_BUILD_EXAMPLES}")
message(STATUS "  Install:        ${VISIONSCENE_INSTALL}")
message(STATUS "  C++ standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "=================================")
message(STATUS "")
