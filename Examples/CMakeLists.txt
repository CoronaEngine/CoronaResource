# Examples build

add_executable(VisionSceneExample main.cpp)





1133include(

)# Normalize source dir path for macro (forward slashes are safe on Windows too)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" VS_SRC_DIR_NORM)

target_compile_definitions(VisionSceneExample PRIVATE VISIONSCENE_SOURCE_DIR="${VS_SRC_DIR_NORM}")

# Link against core library (will be INTERFACE if no sources enabled)
target_link_libraries(VisionSceneExample PRIVATE VisionScene::Core)

target_compile_features(VisionSceneExample PRIVATE cxx_std_17)

set_target_properties(VisionSceneExample PROPERTIES FOLDER Examples)

if(VISIONSCENE_INSTALL)
    install(TARGETS VisionSceneExample RUNTIME DESTINATION bin)
endif()
