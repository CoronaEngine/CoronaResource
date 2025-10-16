# =============================================================================
# CoronaResource External Dependencies
# =============================================================================

include(FetchContent)

# Fetch ktm math library
message(STATUS "Fetching ktm math library...")
FetchContent_Declare(
    ktm
    GIT_REPOSITORY https://github.com/YGXXD/ktm.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)

# Fetch Assimp 3D model import library
message(STATUS "Fetching Assimp library...")
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# Configure Assimp options before making it available
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

# Fetch stb single-file public domain libraries
message(STATUS "Fetching stb library...")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(CoronaLogger
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaLogger.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(CabbageConcurrent
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageConcurrent.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

# Make dependencies available
FetchContent_MakeAvailable(ktm assimp stb CoronaLogger CabbageConcurrent)

# Create interface library for stb (header-only)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

add_library(stb_headers INTERFACE)
target_include_directories(stb_headers INTERFACE ${stb_SOURCE_DIR})

# Status messages
message(STATUS "ktm library: Ready")
message(STATUS "Assimp library: Ready")
message(STATUS "stb library: Ready (header-only)")
