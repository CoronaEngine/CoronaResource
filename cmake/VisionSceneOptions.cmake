# =============================================================================
# VisionScene Build Options
# =============================================================================

# Default values based on whether this is a top-level project
if(NOT DEFINED VISIONSCENE_IS_TOP_LEVEL)
    set(VISIONSCENE_IS_TOP_LEVEL ON)
endif()

# Build options
# When used as a subproject, disable examples and install by default
option(VISIONSCENE_BUILD_EXAMPLES "Build VisionScene examples" ${VISIONSCENE_IS_TOP_LEVEL})
option(VISIONSCENE_INSTALL "Enable install rules" ${VISIONSCENE_IS_TOP_LEVEL})

# Subsystem options
option(VISIONSCENE_BUILD_IMPORT "Build import subsystem (requires external engine headers)" OFF)
option(VISIONSCENE_BUILD_DATASMITH "Build Datasmith parser" ON)
option(VISIONSCENE_BUILD_PBRT_IMPORT "Build PBRT scene importer" ON)
option(VISIONSCENE_BUILD_RESOURCE "Build resource library" ON)

# Library type
# Allow user to choose static/shared via standard BUILD_SHARED_LIBS
# Default to static library if not specified
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
endif()
