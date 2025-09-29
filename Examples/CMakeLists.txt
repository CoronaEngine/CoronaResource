# Examples build

add_executable(VisionSceneExample main.cpp)

# Link against core library (will be INTERFACE if no sources enabled)
target_link_libraries(VisionSceneExample PRIVATE VisionScene::Core)

target_compile_features(VisionSceneExample PRIVATE cxx_std_17)

set_target_properties(VisionSceneExample PROPERTIES FOLDER Examples)

if(VISIONSCENE_INSTALL)
    install(TARGETS VisionSceneExample RUNTIME DESTINATION bin)
endif()

