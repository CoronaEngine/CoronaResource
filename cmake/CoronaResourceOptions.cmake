# =============================================================================
# CoronaResource Build Options
# =============================================================================

# Default values based on whether this is a top-level project
if(NOT DEFINED CORONARESOURCE_IS_TOP_LEVEL)
    set(CORONARESOURCE_IS_TOP_LEVEL ON)
endif()

# Build options
# When used as a subproject, disable examples and install by default
option(CORONARESOURCE_BUILD_EXAMPLES "Build CoronaResource examples" ${CORONARESOURCE_IS_TOP_LEVEL})
option(CORONARESOURCE_INSTALL "Enable install rules" ${CORONARESOURCE_IS_TOP_LEVEL})

# Subsystem options
option(CORONARESOURCE_BUILD_IMPORT "Build import subsystem (requires external engine headers)" OFF)
# Backward-compat: if legacy CORONARESOURCE_BUILD_Mitsuba is defined, map it to the new all-caps name.
if(DEFINED CORONARESOURCE_BUILD_Mitsuba AND NOT DEFINED CORONARESOURCE_BUILD_MITSUBA)
    set(CORONARESOURCE_BUILD_MITSUBA ${CORONARESOURCE_BUILD_Mitsuba})
endif()
option(CORONARESOURCE_BUILD_MITSUBA "Build Mitsuba parser" OFF)
option(CORONARESOURCE_BUILD_DATASMITH "Build Datasmith parser" OFF)
option(CORONARESOURCE_BUILD_PBRT_IMPORT "Build PBRT scene importer" OFF)
option(CORONARESOURCE_BUILD_RESOURCE "Build resource library" ON)

# Upstream parser source toggles (require external SDK headers)
option(CORONARESOURCE_USE_UPSTREAM_MITSUBA "Include src/importer/mitsuba/*.cpp (requires Mitsuba SDK headers)" OFF)
option(CORONARESOURCE_USE_UPSTREAM_PBRT "Include src/importer/pbrt/*.cpp (requires PBRT headers)" OFF)

# Importer subfolders (under src/importer)
option(CORONARESOURCE_BUILD_IMPORTER_UTILS "Build importer/utils (factory, detectors, importers stubs)" ON)
option(CORONARESOURCE_BUILD_TUNGSTEN "Build Tungsten importer (src/importer/tungsten)" OFF)
option(CORONARESOURCE_BUILD_VISION_SCENE "Build VisionScene importer (src/importer/vision_scene)" OFF)
option(CORONARESOURCE_BUILD_IMPORTER_IMPORT "Build core importer (src/importer/import)" OFF)
option(CORONARESOURCE_BUILD_SCENE_LOADER "Build scene_loader entry (src/importer/scene_loader.*)" ON)

# Non-C++ helper folders (kept as toggles for packaging/install; no C++ sources)
option(CORONARESOURCE_BUILD_B2V "Enable Blender-to-Vision addon (src/importer/b2v)" OFF)
option(CORONARESOURCE_BUILD_IMPORTER_TOOLS "Enable importer/tools python utilities" OFF)

# Library type
# Allow user to choose static/shared via standard BUILD_SHARED_LIBS
# Default to static library if not specified
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
endif()
