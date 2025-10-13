# =============================================================================
# VisionScene Utility Functions
# =============================================================================

# Print configuration summary
function(visionscene_print_summary)
    message(STATUS "")
    message(STATUS "=== VisionScene Configuration ===")
    message(STATUS "  Version:        ${PROJECT_VERSION}")
    message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Shared libs:    ${BUILD_SHARED_LIBS}")
    message(STATUS "  Build examples: ${VISIONSCENE_BUILD_EXAMPLES}")
    message(STATUS "  Install:        ${VISIONSCENE_INSTALL}")
    message(STATUS "  C++ standard:   ${CMAKE_CXX_STANDARD}")
    message(STATUS "")
    message(STATUS "  Subsystems:")
    message(STATUS "    Datasmith:    ${VISIONSCENE_BUILD_DATASMITH}")
    message(STATUS "    PBRT:         ${VISIONSCENE_BUILD_PBRT_IMPORT}")
    message(STATUS "    Import:       ${VISIONSCENE_BUILD_IMPORT}")
    message(STATUS "=================================")
    message(STATUS "")
endfunction()

# Configure library properties
function(visionscene_set_target_properties target)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME visionscene
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        FOLDER "VisionScene"
    )
endfunction()

# Add source group for IDE organization
function(visionscene_add_source_group target source_dir)
    get_target_property(sources ${target} SOURCES)
    source_group(TREE ${source_dir} FILES ${sources})
endfunction()
