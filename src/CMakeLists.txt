# =============================================================================
# VisionScene Core Library
# =============================================================================

# Build options for subsystems
option(VISIONSCENE_BUILD_IMPORT "Build import subsystem (requires external engine headers)" OFF)
option(VISIONSCENE_BUILD_DATASMITH "Build Datasmith parser" ON)
option(VISIONSCENE_BUILD_PBRT_IMPORT "Build PBRT scene importer" ON)

# Initialize source lists
set(VISIONSCENE_CORE_SOURCES)
set(VISIONSCENE_CORE_PUBLIC_HEADERS)

# Collect source files based on enabled features

# Datasmith parser
if(VISIONSCENE_BUILD_DATASMITH)
    file(GLOB DATASMITH_REFACTOR_PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Private/*.cpp
    )
    file(GLOB DATASMITH_REFACTOR_PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Public/*.h
    )
    list(APPEND VISIONSCENE_CORE_SOURCES ${DATASMITH_REFACTOR_PRIVATE})
    list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${DATASMITH_REFACTOR_PUBLIC})
    message(STATUS "Datasmith parser: ENABLED")
else()
    message(STATUS "Datasmith parser: DISABLED")
endif()

# Import subsystem (requires external engine headers)
if(VISIONSCENE_BUILD_IMPORT)
    file(GLOB IMPORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/import/*.cpp
    )
    file(GLOB IMPORT_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/import/*.h
    )
    list(APPEND VISIONSCENE_CORE_SOURCES ${IMPORT_SRC})
    list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${IMPORT_HEADERS})
    message(STATUS "Import subsystem: ENABLED")
else()
    message(STATUS "Import subsystem: DISABLED")
endif()

# PBRT scene importer
if(VISIONSCENE_BUILD_PBRT_IMPORT)
    file(GLOB PBRT_IMPORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/pbrt_import/*.cpp
    )
    file(GLOB PBRT_IMPORT_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/pbrt_import/*.h
    )
    list(APPEND VISIONSCENE_CORE_SOURCES ${PBRT_IMPORT_SRC})
    list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${PBRT_IMPORT_HEADERS})
    message(STATUS "PBRT importer: ENABLED")
else()
    message(STATUS "PBRT importer: DISABLED")
endif()

# Utilities (format detection + import factory)
file(GLOB VS_UTILS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)
file(GLOB VS_UTILS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h
)
list(APPEND VISIONSCENE_CORE_SOURCES ${VS_UTILS_SRC})
list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS ${VS_UTILS_HEADERS})

# Generic scene loader (format auto-detection entry point)
list(APPEND VISIONSCENE_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_loader.cpp
)
list(APPEND VISIONSCENE_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_loader.h
)

# Verify we have sources to build
if(NOT VISIONSCENE_CORE_SOURCES)
    message(WARNING "No core sources enabled (all subsystems disabled). Creating header-only interface target.")
    add_library(VisionSceneCore INTERFACE)
    target_include_directories(VisionSceneCore
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    add_library(VisionScene::Core ALIAS VisionSceneCore)
    return()
endif()

# Create the library
add_library(VisionSceneCore ${VISIONSCENE_CORE_SOURCES} ${VISIONSCENE_CORE_PUBLIC_HEADERS})
add_library(VisionScene::Core ALIAS VisionSceneCore)

set_target_properties(VisionSceneCore PROPERTIES
    OUTPUT_NAME visionscene
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Generate version header
set(VISIONSCENE_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${VISIONSCENE_GENERATED_DIR})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/VisionSceneVersion.h.in
    ${VISIONSCENE_GENERATED_DIR}/VisionSceneVersion.h
    @ONLY
)

# Configure include directories
# Use generator expressions for export-safe paths:
# - BUILD_INTERFACE: paths visible during build
# - INSTALL_INTERFACE: paths visible after installation
target_include_directories(VisionSceneCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/datasmith_refactor/Public>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pbrt_import>
        $<BUILD_INTERFACE:${VISIONSCENE_GENERATED_DIR}>
        $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions for feature toggles
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

if(VISIONSCENE_BUILD_PBRT_IMPORT)
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_PBRT=1)
else()
    target_compile_definitions(VisionSceneCore PUBLIC VISIONSCENE_ENABLE_PBRT=0)
endif()

# Link dependencies
target_link_libraries(VisionSceneCore
    PUBLIC
        ktm
)

# IDE source organization
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${VISIONSCENE_CORE_SOURCES} ${VISIONSCENE_CORE_PUBLIC_HEADERS}
)

# Installation rules
if(VISIONSCENE_INSTALL)
    # Install library targets
    install(
        TARGETS VisionSceneCore
        EXPORT VisionSceneTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
    
    # Install public headers
    if(VISIONSCENE_CORE_PUBLIC_HEADERS)
        install(
            FILES ${VISIONSCENE_CORE_PUBLIC_HEADERS}
            DESTINATION include/VisionScene
        )
    endif()
    
    # Install generated version header
    install(
        FILES ${VISIONSCENE_GENERATED_DIR}/VisionSceneVersion.h
        DESTINATION include/VisionScene
    )
endif()
