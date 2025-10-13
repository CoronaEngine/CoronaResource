# =============================================================================
# VisionScene Build Options
# =============================================================================

# Build options
option(VISIONSCENE_BUILD_EXAMPLES "Build VisionScene examples" ON)
option(VISIONSCENE_INSTALL "Enable install rules" ON)

# Subsystem options
option(VISIONSCENE_BUILD_IMPORT "Build import subsystem (requires external engine headers)" OFF)
option(VISIONSCENE_BUILD_DATASMITH "Build Datasmith parser" ON)
option(VISIONSCENE_BUILD_PBRT_IMPORT "Build PBRT scene importer" ON)

# Library type
# Allow user to choose static/shared via standard BUILD_SHARED_LIBS
# Default to static library if not specified
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
endif()
