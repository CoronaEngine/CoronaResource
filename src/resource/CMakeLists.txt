# 文件: src/resource/CMakeLists.txt
# 目标: VisionSceneResource
# 描述: VisionScene 项目资源库

message(STATUS "[VisionScene] Configuring Resource library")

# 收集源文件和头文件
file(GLOB_RECURSE RESOURCE_PRIVATE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/private/*.cpp"
)

file(GLOB_RECURSE RESOURCE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/public/*.h"
)

# 创建静态库
add_library(VisionSceneResource STATIC)
add_library(VisionScene::Resource ALIAS VisionSceneResource)

# 添加源文件和头文件
if(RESOURCE_PRIVATE_SOURCES)
    target_sources(VisionSceneResource PRIVATE ${RESOURCE_PRIVATE_SOURCES})
    message(STATUS "[Resource] Added ${CMAKE_CURRENT_LIST_LENGTH} private sources")
endif()

if(RESOURCE_PUBLIC_HEADERS)
    target_sources(VisionSceneResource PUBLIC ${RESOURCE_PUBLIC_HEADERS})
    message(STATUS "[Resource] Added ${CMAKE_CURRENT_LIST_LENGTH} public headers")
endif()

# 设置 C++20 标准
target_compile_features(VisionSceneResource PUBLIC cxx_std_20)

# 配置包含目录
target_include_directories(VisionSceneResource
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
    $<INSTALL_INTERFACE:include/VisionScene/Resource>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
    ${ktm_SOURCE_DIR}
)

# 设置目标属性
set_target_properties(VisionSceneResource PROPERTIES
    FOLDER "VisionScene/Libraries"
    OUTPUT_NAME "VisionSceneResource"
)

# 链接依赖库
target_link_libraries(VisionSceneResource
    PUBLIC
    stb_headers
    assimp::assimp
    INTERFACE
    ktm
)

message(STATUS "[Resource] Configuration complete")