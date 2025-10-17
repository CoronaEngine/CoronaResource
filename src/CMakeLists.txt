# =============================================================================
# CoronaResource Core Library
# =============================================================================

# Initialize source lists
set(CORONARESOURCE_CORE_SOURCES)
set(CORONARESOURCE_CORE_PUBLIC_HEADERS)

set(CORONARESOURCE_IMPORTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/importer)

# Collect source files based on enabled features

# Datasmith parser
if(CORONARESOURCE_BUILD_DATASMITH)
    file(GLOB DATASMITH_REFACTOR_PRIVATE
        ${CORONARESOURCE_IMPORTER_DIR}/datasmith_refactor/Private/*.cpp
    )
    file(GLOB DATASMITH_REFACTOR_PUBLIC
        ${CORONARESOURCE_IMPORTER_DIR}/datasmith_refactor/Public/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${DATASMITH_REFACTOR_PRIVATE})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${DATASMITH_REFACTOR_PUBLIC})
    message(STATUS "Datasmith parser: ENABLED")
else()
    message(STATUS "Datasmith parser: DISABLED")
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

# PBRT scene importer
if(CORONARESOURCE_BUILD_PBRT_IMPORT)
    file(GLOB PBRT_IMPORT_SRC
        ${CORONARESOURCE_IMPORTER_DIR}/pbrt/*.cpp
    )
    file(GLOB PBRT_IMPORT_HEADERS
        ${CORONARESOURCE_IMPORTER_DIR}/pbrt/*.h
    )
    list(APPEND CORONARESOURCE_CORE_SOURCES ${PBRT_IMPORT_SRC})
    list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${PBRT_IMPORT_HEADERS})
    message(STATUS "PBRT importer: ENABLED")
else()
    message(STATUS "PBRT importer: DISABLED")
endif()

# Utilities (format detection + import factory)
file(GLOB VS_UTILS_SRC
    ${CORONARESOURCE_IMPORTER_DIR}/utils/private/*.cpp
)
file(GLOB VS_UTILS_HEADERS
    ${CORONARESOURCE_IMPORTER_DIR}/utils/public/*.h
)
list(APPEND CORONARESOURCE_CORE_SOURCES ${VS_UTILS_SRC})
list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS ${VS_UTILS_HEADERS})

# Generic scene loader (format auto-detection entry point)
list(APPEND CORONARESOURCE_CORE_SOURCES
    ${CORONARESOURCE_IMPORTER_DIR}/scene_loader.cpp
)
list(APPEND CORONARESOURCE_CORE_PUBLIC_HEADERS
    ${CORONARESOURCE_IMPORTER_DIR}/scene_loader.h
)

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
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/datasmith_refactor/Public>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/pbrt>
        $<BUILD_INTERFACE:${CORONARESOURCE_IMPORTER_DIR}/utils/public>
        $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions for feature toggles
if(CORONARESOURCE_BUILD_IMPORT)
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_IMPORT=1)
else()
    target_compile_definitions(CoronaResourceCore PUBLIC CORONARESOURCE_ENABLE_IMPORT=0)
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
