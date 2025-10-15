# 文件: src/resource/CMakeLists.txt
# 目标: CoronaResource
# 描述: CoronaResource 项目资源库

message(STATUS "[CoronaResource] Configuring Resource library")

# 收集源文件和头文件
file(GLOB_RECURSE RESOURCE_PRIVATE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/private/*.cpp"
)

file(GLOB_RECURSE RESOURCE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/public/*.h"
)

# 创建静态库
add_library(CoronaResource STATIC)
add_library(CoronaResource::Resource ALIAS CoronaResource)

# 添加源文件和头文件
if(RESOURCE_PRIVATE_SOURCES)
    target_sources(CoronaResource PRIVATE ${RESOURCE_PRIVATE_SOURCES})
    message(STATUS "[Resource] Added ${CMAKE_CURRENT_LIST_LENGTH} private sources")
endif()

if(RESOURCE_PUBLIC_HEADERS)
    target_sources(CoronaResource PUBLIC ${RESOURCE_PUBLIC_HEADERS})
    message(STATUS "[Resource] Added ${CMAKE_CURRENT_LIST_LENGTH} public headers")
endif()

# 设置 C++20 标准
target_compile_features(CoronaResource PUBLIC cxx_std_20)

# 配置包含目录
target_include_directories(CoronaResource
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
    $<INSTALL_INTERFACE:include/CoronaResource/Resource>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
    ${ktm_SOURCE_DIR}
)

# 设置目标属性
set_target_properties(CoronaResource PROPERTIES
    FOLDER "CoronaResource/Libraries"
    OUTPUT_NAME "CoronaResource"
)

# 链接依赖库
target_link_libraries(CoronaResource
    PUBLIC
    stb_headers
    assimp::assimp
    INTERFACE
    ktm
)

message(STATUS "[Resource] Configuration complete")