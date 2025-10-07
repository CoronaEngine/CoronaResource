cmake_minimum_required(VERSION 3.20)

# Project definition
project(VisionScene
    VERSION 0.5.0
    DESCRIPTION "VisionScene: Datasmith scene parsing and import core"
    LANGUAGES CXX)

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(VISIONSCENE_BUILD_EXAMPLES "Build VisionScene examples" ON)
option(VISIONSCENE_INSTALL "Enable install rules" ON)

# Allow user to choose static/shared via standard BUILD_SHARED_LIBS
# (Default static if not specified)
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

# Warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

FetchContent_Declare(
        ktm
        GIT_REPOSITORY https://github.com/YGXXD/ktm.git
        GIT_TAG main
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ktm)

add_subdirectory(Src)
if(VISIONSCENE_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# Install (headers handled in Src/CMakeLists)
if(VISIONSCENE_INSTALL)
    include(CMakePackageConfigHelpers)
    set(config_install_dir lib/cmake/VisionScene)
    set(VISIONSCENE_EXPORT_NAME VisionSceneTargets)

    install(EXPORT ${VISIONSCENE_EXPORT_NAME}
        NAMESPACE VisionScene::
        DESTINATION ${config_install_dir}
    )

    # Config file
    configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/VisionSceneConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
        INSTALL_DESTINATION ${config_install_dir}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        DESTINATION ${config_install_dir})
endif()

# Summary message
message(STATUS "VisionScene ${PROJECT_VERSION} | Shared: ${BUILD_SHARED_LIBS} | Examples: ${VISIONSCENE_BUILD_EXAMPLES}")
