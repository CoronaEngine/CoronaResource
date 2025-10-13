# =============================================================================
# VisionScene Examples
# =============================================================================

# Main example executable
add_executable(VisionSceneExample
    main.cpp
)

# Link against core library
# Note: Will be INTERFACE target if no sources are enabled
target_link_libraries(VisionSceneExample
    PRIVATE
        VisionScene::Core
)

# Set C++ standard
target_compile_features(VisionSceneExample
    PRIVATE
        cxx_std_20
)

# IDE folder organization
set_target_properties(VisionSceneExample
    PROPERTIES
        FOLDER "Examples"
        OUTPUT_NAME "visionscene_example"
)

# Installation
if(VISIONSCENE_INSTALL)
    install(
        TARGETS VisionSceneExample
        RUNTIME DESTINATION ${VISIONSCENE_INSTALL_BINDIR}
    )
endif()
