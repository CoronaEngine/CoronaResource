# =============================================================================
# CoronaResource Core Library
# =============================================================================

# Initialize source lists
set(CORONARESOURCE_CORE_SOURCES)
set(CORONARESOURCE_CORE_PUBLIC_HEADERS)

set(CORONARESOURCE_IMPORTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/importer)

# Collect source files based on enabled features

# Datasmith parser (note: currently no C++ sources in importer/datasmith)
if(CORONARESOURCE_BUILD_DATASMITH)
    file(GLOB DATASMITH_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/datasmith/*.cpp
    )
    file(GLOB DATASMITH_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/datasmith/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${DATASMITH_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${DATASMITH_HEADERS})
    message(STATUS "Datasmith parser: ENABLED (sources found: ${DATASMITH_SRC})")
else()
    message(STATUS "Datasmith parser: DISABLED")
endif()

# Mitsuba parser (gate upstream sources to avoid external SDK unless explicitly enabled)
if(CORONARESOURCE_BUILD_MITSUBA)
    if(CORONARESOURCE_USE_UPSTREAM_MITSUBA)
        file(GLOB MITSUBA_SRC
            ${CORONARESOURCE_IMPORTER_DIR}/mitsuba/*.cpp
        )
        file(GLOB MITSUBA_HEADERS
            ${CORONARESOURCE_IMPORTER_DIR}/mitsuba/*.h
        )
        list(APPEND CORONARESOURCE_CORE_SOURCES ${MITSUBA_SRC})
        list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${MITSUBA_HEADERS})
        message(STATUS "Mitsuba parser (upstream sources): ENABLED")
    else()
        message(STATUS "Mitsuba parser: ENABLED (utils-only; upstream sources disabled)")
    endif()
else()
    message(STATUS "Mitsuba parser: DISABLED")
endif()

# Import subsystem (requires external engine headers)
if(CORONARESOURCE_BUILD_IMPORT)
    file(GLOB IMPORT_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/import/*.cpp
    )
    file(GLOB IMPORT_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/import/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${IMPORT_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${IMPORT_HEADERS})
    message(STATUS "Import subsystem: ENABLED")
else()
    message(STATUS "Import subsystem: DISABLED")
endif()

# PBRT scene importer (gate upstream sources)
if(CORONARESOURCE_BUILD_PBRT_IMPORT)
    if(CORONARESOURCE_USE_UPSTREAM_PBRT)
        file(GLOB PBRT_IMPORT_SRC
            ${CORONARESOURCE_IMPORTER_DIR}/pbrt/*.cpp
        )
        file(GLOB PBRT_IMPORT_HEADERS
            ${CORONARESOURCE_IMPORTER_DIR}/pbrt/*.h
        )
        list(APPEND CORONARESOURCE_CORE_SOURCES ${PBRT_IMPORT_SRC})
        list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${PBRT_IMPORT_HEADERS})
        message(STATUS "PBRT importer (upstream sources): ENABLED")
    else()
        message(STATUS "PBRT importer: ENABLED (utils-only; upstream sources disabled)")
    endif()
else()
    message(STATUS "PBRT importer: DISABLED")
endif()

# Utilities (format detection + import factory)
if(CORONARESOURCE_BUILD_IMPORTER_UTILS)
    file(GLOB VS_UTILS_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/utils/private/*.cpp
    )
    file(GLOB VS_UTILS_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/utils/public/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${VS_UTILS_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${VS_UTILS_HEADERS})
    message(STATUS "Importer utils: ENABLED")
else()
    message(STATUS "Importer utils: DISABLED")
endif()

# Tungsten importer
if(CORONARESOURCE_BUILD_TUNGSTEN)
    file(GLOB TUNGSTEN_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/tungsten/*.cpp
    )
    file(GLOB TUNGSTEN_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/tungsten/*.hpp
        ${CORONARESOURCE_IMPORTER_DIR}/tungsten/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${TUNGSTEN_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${TUNGSTEN_HEADERS})
    message(STATUS "Tungsten importer: ENABLED")
else()
    message(STATUS "Tungsten importer: DISABLED")
endif()

# VisionScene importer (C++ IR types)
if(CORONARESOURCE_BUILD_VISION_SCENE)
    file(GLOB VISION_SCENE_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/vision_scene/Private/*.cpp
    )
    file(GLOB VISION_SCENE_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/vision_scene/Public/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${VISION_SCENE_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${VISION_SCENE_HEADERS})
    message(STATUS "VisionScene importer: ENABLED")
else()
    message(STATUS "VisionScene importer: DISABLED")
endif()

# Scene loader entry point
if(CORONARESOURCE_BUILD_SCENE_LOADER)
    list(APPEND CORONARESOURCE_CORE_SOURCES
        ${CORONARESOURCE_IMPORTER_DIR}/scene_loader.cpp
    )
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/scene_loader.h
    )
    message(STATUS "Scene loader: ENABLED")
else()
    message(STATUS "Scene loader: DISABLED")
endif()

# Verify we have sources to build
if(NOT CORONARESOURCE_CORE_SOURCES)
    message(WARNING "No core sources enabled (all subsystems disabled). Creating header-only interface target.")
    add_library(CoronaResourceCore INTERFACE)
    target_include_directories(CoronaResourceCore
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    add_library(CoronaResource::Core ALIAS CoronaResourceCore)
    return()
endif()

# Create the library
add_library(CoronaResourceCore ${CORONARESOURCE_CORE_SOURCES} ${CORONARESOURCE_CORE_PUBLIC_HEADERS})
add_library(CoronaResource::Core ALIAS CoronaResourceCore)

# Set library properties using utility function
coronaresource_set_target_properties(CoronaResourceCore)

# Configure include directories
# Use generator expressions for export-safe paths:
# - BUILD_INTERFACE: paths visible during build
# - INSTALL_INTERFACE: paths visible after installation
target_include_directories(CoronaResourceCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/mitsuba>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/datasmith>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/pbrt>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/utils/public>
        $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Ensure nlohmann/json headers are visible to consumers/IDEs when using FetchContent
if(DEFINED nlohmann_json_SOURCE_DIR)
    target_include_directories(CoronaResourceCore PUBLIC $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>)
endif()

# Compile definitions for feature toggles
if(CORONARESOURCE_BUILD_IMPORT)
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_IMPORT=1)
else()
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_IMPORT=0)
endif()

if(CORONARESOURCE_BUILD_MITSUBA)
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_MITSUBA=1)
else()
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_MITSUBA=0)
endif()

if(CORONARESOURCE_BUILD_DATASMITH)
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_DATASMITH=1)
else()
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_DATASMITH=0)
endif()

if(CORONARESOURCE_BUILD_PBRT_IMPORT)
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_PBRT=1)
else()
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_PBRT=0)
endif()

# Link dependencies
target_link_libraries(CoronaResourceCore
    PUBLIC
        ktm
        nlohmann_json::nlohmann_json
)

# IDE source organization using utility function
coronaresource_add_source_group(CoronaResourceCore ${CMAKE_CURRENT_SOURCE_DIR})

# Installation rules
if(CORONARESOURCE_INSTALL)
    # Install library targets
    install(
        TARGETS CoronaResourceCore
        EXPORT ${CORONARESOURCE_EXPORT_NAME}
        ARCHIVE DESTINATION ${CORONARESOURCE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CORONARESOURCE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CORONARESOURCE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CORONARESOURCE_INSTALL_INCLUDEDIR}
    )
    
    # Install public headers
    if(CORONARESOURCE_CORE_PUBLIC_HEADERS)
        install(
            FILES ${CORONARESOURCE_CORE_PUBLIC_HEADERS}
            DESTINATION ${CORONARESOURCE_INSTALL_INCLUDEDIR}/CoronaResource
        )
    endif()
endif()

# =============================================================================
# Resource Library (Optional)
# =============================================================================
if(CORONARESOURCE_BUILD_RESOURCE)
    add_subdirectory(resource)
    message(STATUS "[CoronaResource] Resource library included")
else()
    message(STATUS "[CoronaResource] Resource library disabled")
endif()
