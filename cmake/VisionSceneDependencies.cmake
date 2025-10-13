# =============================================================================
# VisionScene External Dependencies
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

FetchContent_MakeAvailable(ktm)
message(STATUS "ktm library: Ready")
