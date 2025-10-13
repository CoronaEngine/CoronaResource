# =============================================================================
# VisionScene Compiler Settings
# =============================================================================

# Global C++ standard
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# IDE folder organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler-specific warnings and flags
if(MSVC)
    add_compile_options(
        /W4 # Warning level 4
        /permissive- # Standards conformance
        $<$<COMPILE_LANGUAGE:C>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:C>:/execution-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/execution-charset:utf-8>
    )
else()
    add_compile_options(
        -Wall # Enable all warnings
        -Wextra # Extra warnings
        -Wpedantic # Pedantic warnings
    )
endif()
