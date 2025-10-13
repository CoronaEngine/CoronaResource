# 文件: src/resource_manager/CMakeLists.txt
# 目标: VisionSceneResourceManager
# 描述: VisionScene 项目资源管理库

message(STATUS "[VisionScene] Configuring ResourceManager library")

# 收集源文件和头文件
file(GLOB_RECURSE RESOURCE_MANAGER_PRIVATE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/private/*.cpp"
)

file(GLOB_RECURSE RESOURCE_MANAGER_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/public/*.h"
)

# 创建静态库
add_library(VisionSceneResourceManager STATIC)
add_library(VisionScene::ResourceManager ALIAS VisionSceneResourceManager)

# 添加源文件和头文件
if(RESOURCE_MANAGER_PRIVATE_SOURCES)
    target_sources(VisionSceneResourceManager PRIVATE ${RESOURCE_MANAGER_PRIVATE_SOURCES})
    message(STATUS "[ResourceManager] Added ${CMAKE_CURRENT_LIST_LENGTH} private sources")
endif()

if(RESOURCE_MANAGER_PUBLIC_HEADERS)
    target_sources(VisionSceneResourceManager PUBLIC ${RESOURCE_MANAGER_PUBLIC_HEADERS})
    message(STATUS "[ResourceManager] Added ${CMAKE_CURRENT_LIST_LENGTH} public headers")
endif()

# 设置 C++20 标准
target_compile_features(VisionSceneResourceManager PUBLIC cxx_std_20)

# 配置包含目录
target_include_directories(VisionSceneResourceManager
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
    $<INSTALL_INTERFACE:include/VisionScene/ResourceManager>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
    ${ktm_SOURCE_DIR}
)

# 设置目标属性
set_target_properties(VisionSceneResourceManager PROPERTIES
    FOLDER "VisionScene/Libraries"
    OUTPUT_NAME "VisionSceneResourceManager"
)

# 链接依赖库
target_link_libraries(VisionSceneResourceManager
    PUBLIC
    stb_headers
    assimp::assimp
    INTERFACE
    ktm
)

message(STATUS "[ResourceManager] Configuration complete")