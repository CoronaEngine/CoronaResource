# =============================================================================
# CoronaResource Installation Configuration
# =============================================================================

include(CMakePackageConfigHelpers)

# Installation paths
set(CORONARESOURCE_INSTALL_BINDIR bin CACHE STRING "Binary installation directory")
set(CORONARESOURCE_INSTALL_LIBDIR lib CACHE STRING "Library installation directory")
set(CORONARESOURCE_INSTALL_INCLUDEDIR include CACHE STRING "Include installation directory")
set(CORONARESOURCE_INSTALL_CMAKEDIR lib/cmake/CoronaResource CACHE STRING "CMake config installation directory")

# Export target name
set(CORONARESOURCE_EXPORT_NAME CoronaResourceTargets)

# Function to configure installation
function(coronaresource_configure_install)
    if(NOT CORONARESOURCE_INSTALL)
        return()
    endif()

    # If present, include third-party targets required by CoronaResourceCore in the export set
    if(TARGET nlohmann_json)
        install(TARGETS nlohmann_json EXPORT ${CORONARESOURCE_EXPORT_NAME})
    endif()

    # Install export targets
    install(
        EXPORT ${CORONARESOURCE_EXPORT_NAME}
        NAMESPACE CoronaResource::
        DESTINATION ${CORONARESOURCE_INSTALL_CMAKEDIR}
    )

    # Generate and install config files
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoronaResourceConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/CoronaResourceConfig.cmake
        INSTALL_DESTINATION ${CORONARESOURCE_INSTALL_CMAKEDIR}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/CoronaResourceConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/CoronaResourceConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/CoronaResourceConfigVersion.cmake
        DESTINATION ${CORONARESOURCE_INSTALL_CMAKEDIR}
    )
endfunction()
