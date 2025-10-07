# CMake build script for core C++ parts of VisionScene

# Options for selectively building subsystems (import depends on external headers not in repo)
option(VISIONSCENE_BUILD_IMPORT "Build import subsystem (requires external engine headers)" OFF)
option(VISIONSCENE_BUILD_DATASMITH "Build datasmith refactor sources" ON)

set(VISIONSCENE_CORE_SOURCES)
set(VISIONSCENE_CORE_PUBLIC_HEADERS)

# Datasmith refactor sources
if(VISIONSCENE_BUILD_DATASMITH)
    file(GLOB DATASMITH_REFACTOR_PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Private/*.cpp)
    file(GLOB DATASMITH_REFACTOR_PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Public/*.h)
    list(APPEND VISIONSCENE_CORE_SOURCES ${DATASMITH_REFACTOR_PRIVATE})
    list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${DATASMITH_REFACTOR_PUBLIC})
endif()

# Import subsystem (guarded due to missing external includes in repo)
if(VISIONSCENE_BUILD_IMPORT)
    file(GLOB IMPORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/import/*.cpp)
    file(GLOB IMPORT_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/import/*.h)
    list(APPEND VISIONSCENE_CORE_SOURCES ${IMPORT_SRC})
    list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${IMPORT_HEADERS})
endif()

# Fallback if no sources selected
if(NOT VISIONSCENE_CORE_SOURCES)
    message(WARNING "No core sources enabled (both DATASMITH and IMPORT disabled). Creating interface target.")
    add_library(VisionSceneCore INTERFACE)
    target_include_directories(VisionSceneCore INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    add_library(VisionScene::Core ALIAS VisionSceneCore)
    return()
endif()

add_library(VisionSceneCore ${VISIONSCENE_CORE_SOURCES} ${VISIONSCENE_CORE_PUBLIC_HEADERS})
add_library(VisionScene::Core ALIAS VisionSceneCore)

# Generate version header
set(VISIONSCENE_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${VISIONSCENE_GENERATED_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VisionSceneVersion.h.in
               ${VISIONSCENE_GENERATED_DIR}/VisionSceneVersion.h @ONLY)

# Public include dirs (export-safe)
# Use generator expressions so source/build paths are only visible while building, not after installation.
target_include_directories(VisionSceneCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Public>
        $<BUILD_INTERFACE:${VISIONSCENE_GENERATED_DIR}>
        $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Basic compile definitions
if(VISIONSCENE_BUILD_IMPORT)
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_IMPORT=1)
else()
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_IMPORT=0)
endif()
if(VISIONSCENE_BUILD_DATASMITH)
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_DATASMITH=1)
else()
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_DATASMITH=0)
endif()

# Organize sources into folders (for IDEs)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VISIONSCENE_CORE_SOURCES} ${VISIONSCENE_CORE_PUBLIC_HEADERS})

# Installation rules
if(VISIONSCENE_INSTALL)
    install(TARGETS VisionSceneCore EXPORT VisionSceneTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
    # Install headers (public + generated version header)
    if(VISIONSCENE_CORE_PUBLIC_HEADERS)
        install(FILES ${VISIONSCENE_CORE_PUBLIC_HEADERS}
            DESTINATION include/datasmith_refactor)
    endif()
    install(FILES ${VISIONSCENE_GENERATED_DIR}/VisionSceneVersion.h
        DESTINATION include/VisionScene)
endif()
