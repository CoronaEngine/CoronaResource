cmake_minimum_required(VERSION 4.0)

# Project definition
project(CoronaResource
    VERSION 0.5.0
    DESCRIPTION "CoronaResource: Multi-format 3D scene parsing and import library"
    LANGUAGES CXX
)

# Detect if CoronaResource is being used as a subproject
if(NOT DEFINED CORONARESOURCE_IS_TOP_LEVEL)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(CORONARESOURCE_IS_TOP_LEVEL ON)
    else()
        set(CORONARESOURCE_IS_TOP_LEVEL OFF)
    endif()
endif()

# Add CMake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include configuration modules
include(CoronaResourceOptions)
include(CoronaResourceCompiler)
include(CoronaResourceDependencies)
include(CoronaResourceInstall)
include(CoronaResourceUtils)

# Add subdirectories
add_subdirectory(src)

if(CORONARESOURCE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Configure installation
coronaresource_configure_install()

# Print configuration summary
coronaresource_print_summary()
