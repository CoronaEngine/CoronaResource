# =============================================================================
# VisionScene Installation Configuration
# =============================================================================

include(CMakePackageConfigHelpers)

# Installation paths
set(VISIONSCENE_INSTALL_BINDIR bin CACHE STRING "Binary installation directory")
set(VISIONSCENE_INSTALL_LIBDIR lib CACHE STRING "Library installation directory")
set(VISIONSCENE_INSTALL_INCLUDEDIR include CACHE STRING "Include installation directory")
set(VISIONSCENE_INSTALL_CMAKEDIR lib/cmake/VisionScene CACHE STRING "CMake config installation directory")

# Export target name
set(VISIONSCENE_EXPORT_NAME VisionSceneTargets)

# Function to configure installation
function(visionscene_configure_install)
    if(NOT VISIONSCENE_INSTALL)
        return()
    endif()

    # Install export targets
    install(
        EXPORT ${VISIONSCENE_EXPORT_NAME}
        NAMESPACE VisionScene::
        DESTINATION ${VISIONSCENE_INSTALL_CMAKEDIR}
    )

    # Generate and install config files
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VisionSceneConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
        INSTALL_DESTINATION ${VISIONSCENE_INSTALL_CMAKEDIR}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/VisionSceneConfigVersion.cmake
        DESTINATION ${VISIONSCENE_INSTALL_CMAKEDIR}
    )
endfunction()
