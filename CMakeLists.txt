cmake_minimum_required(VERSION 4.0)

# Project definition
project(VisionScene
    VERSION 0.5.0
    DESCRIPTION "VisionScene: Multi-format 3D scene parsing and import library"
    LANGUAGES CXX
)

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
