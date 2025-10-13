cmake_minimum_required(VERSION 4.0)

# Project definition
project(VisionScene
    VERSION 0.5.0
    DESCRIPTION "VisionScene: Multi-format 3D scene parsing and import library"
    LANGUAGES CXX
)

# Detect if VisionScene is being used as a subproject
if(NOT DEFINED VISIONSCENE_IS_TOP_LEVEL)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(VISIONSCENE_IS_TOP_LEVEL ON)
    else()
        set(VISIONSCENE_IS_TOP_LEVEL OFF)
    endif()
endif()

# Add CMake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include configuration modules
include(VisionSceneOptions)
include(VisionSceneCompiler)
include(VisionSceneDependencies)
include(VisionSceneInstall)
include(VisionSceneUtils)

# Add subdirectories
add_subdirectory(src)

if(VISIONSCENE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Configure installation
visionscene_configure_install()

# Print configuration summary
visionscene_print_summary()
