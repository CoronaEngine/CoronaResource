# =============================================================================
# CoronaResource Utility Functions
# =============================================================================

# Print configuration summary
function(coronaresource_print_summary)
    message(STATUS "")
    message(STATUS "=== CoronaResource Configuration ===")
    message(STATUS "  Version:        ${PROJECT_VERSION}")
    message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Shared libs:    ${BUILD_SHARED_LIBS}")
    message(STATUS "  Build examples: ${CORONARESOURCE_BUILD_EXAMPLES}")
    message(STATUS "  Install:        ${CORONARESOURCE_INSTALL}")
    message(STATUS "  C++ standard:   ${CMAKE_CXX_STANDARD}")
    message(STATUS "")
    message(STATUS "  Subsystems:")
    message(STATUS "    Datasmith:    ${CORONARESOURCE_BUILD_DATASMITH}")
    message(STATUS "    Mitsuba:      ${CORONARESOURCE_BUILD_MITSUBA}")
    message(STATUS "    PBRT:         ${CORONARESOURCE_BUILD_PBRT_IMPORT}")
    message(STATUS "    Tungsten:     ${CORONARESOURCE_BUILD_TUNGSTEN}")
    message(STATUS "    VisionScene:  ${CORONARESOURCE_BUILD_VISION_SCENE}")
    message(STATUS "    Import:       ${CORONARESOURCE_BUILD_IMPORT}")
    message(STATUS "=================================")
    message(STATUS "")
endfunction()

# Configure library properties
function(coronaresource_set_target_properties target)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME coronaresource
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        FOLDER "CoronaResource"
    )
endfunction()

# Add source group for IDE organization
function(coronaresource_add_source_group target source_dir)
    get_target_property(sources ${target} SOURCES)
    source_group(TREE ${source_dir} FILES ${sources})
endfunction()
