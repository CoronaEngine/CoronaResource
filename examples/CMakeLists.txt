# =============================================================================
# CoronaResource Examples
# =============================================================================

# Main example executable
add_executable(CoronaResourceExample
    main.cpp
)

# Link against core library
# Note: Will be INTERFACE target if no sources are enabled
target_link_libraries(CoronaResourceExample
    PRIVATE
        CoronaResource::Core
        CoronaResource::Resource
)

# Set C++ standard
target_compile_features(CoronaResourceExample
    PRIVATE
        cxx_std_20
)

# Provide project source dir to the example for locating test scenes
target_compile_definitions(CoronaResourceExample
    PRIVATE
        CORONARESOURCE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"
)

# IDE folder organization
set_target_properties(CoronaResourceExample
    PROPERTIES
        FOLDER "Examples"
        OUTPUT_NAME "coronaresource_example"
)

# Installation
if(CORONARESOURCE_INSTALL)
    install(
        TARGETS CoronaResourceExample
        RUNTIME DESTINATION ${CORONARESOURCE_INSTALL_BINDIR}
    )
endif()
