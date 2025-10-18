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

message(STATUS "Fetching ")

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

# Fetch nlohmann/json single-header JSON library
message(STATUS "Fetching nlohmann/json library...")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# CoronaLogger
message(STATUS "Fetching CoronaLogger library...")
FetchContent_Declare(
    CoronaLogger
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaLogger.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# CabbageConcurrent
message(STATUS "Fetching CabbageConcurrent library...")
FetchContent_Declare(
    CabbageConcurrent
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageConcurrent.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# Make dependencies available
FetchContent_MakeAvailable(ktm assimp stb nlohmann_json CoronaLogger CabbageConcurrent)

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
message(STATUS "nlohmann/json library: Ready (target: nlohmann_json::nlohmann_json)")
message(STATUS "CoronaLogger: Ready")
message(STATUS "CabbageConcurrent: Ready")
