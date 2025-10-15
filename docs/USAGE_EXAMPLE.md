# CoronaResource 使用示例

这是一个最小化的示例项目，展示如何通过 FetchContent 使用 CoronaResource。

## 项目结构

```
example-project/
├── CMakeLists.txt
└── main.cpp
```

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(CoronaResourceExample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 获取 CoronaResource
FetchContent_Declare(
    CoronaResource
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaResource.git
    GIT_TAG        main
)

# 配置选项
set(CORONARESOURCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CORONARESOURCE_BUILD_RESOURCE ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(CoronaResource)

# 创建可执行文件
add_executable(example main.cpp)

# 链接库
target_link_libraries(example
    PRIVATE
        CoronaResource::Core
        CoronaResource::Resource
)
```

## main.cpp

```cpp
#include <iostream>
#include <Model.h>
#include <ResourceTypes.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <模型文件路径>" << std::endl;
        return 1;
    }

    std::string modelPath = argv[1];
    
    // 创建模型加载器
    auto loader = std::make_shared<vs::resource::ModelLoader>();
    
    // 加载模型
    auto resourceId = vs::resource::ResourceId::create("model", modelPath);
    auto model = loader->load(resourceId, modelPath);
    
    if (!model) {
        std::cerr << "加载模型失败: " << modelPath << std::endl;
        return 1;
    }
    
    // 显示模型信息
    std::cout << "模型加载成功!" << std::endl;
    std::cout << "网格数量: " << model->meshes.size() << std::endl;
    std::cout << "纹理数量: " << model->texturesLoaded.size() << std::endl;
    
    // 显示每个网格的详细信息
    for (size_t i = 0; i < model->meshes.size(); ++i) {
        const auto& mesh = model->meshes[i];
        std::cout << "网格 " << i << ":" << std::endl;
        std::cout << "  顶点数: " << mesh.points.size() / 3 << std::endl;
        std::cout << "  索引数: " << mesh.Indices.size() << std::endl;
        std::cout << "  纹理数: " << mesh.textures.size() << std::endl;
    }
    
    // 显示包围盒信息
    if (model->meshes.size() > 0) {
        const auto& mesh = model->meshes[0];
        std::cout << "包围盒:" << std::endl;
        std::cout << "  最小值: (" 
                  << mesh.BoundingBox.min.x << ", "
                  << mesh.BoundingBox.min.y << ", "
                  << mesh.BoundingBox.min.z << ")" << std::endl;
        std::cout << "  最大值: (" 
                  << mesh.BoundingBox.max.x << ", "
                  << mesh.BoundingBox.max.y << ", "
                  << mesh.BoundingBox.max.z << ")" << std::endl;
    }
    
    return 0;
}
```

## 构建和运行

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 构建
cmake --build .

# 运行（替换为你的模型文件路径）
./example path/to/your/model.obj
```

## 支持的模型格式

通过 Assimp，CoronaResource 支持多种 3D 模型格式：

- OBJ
- FBX
- GLTF/GLB
- DAE (Collada)
- 3DS
- BLEND
- 等等...

## 更多示例

查看 [INTEGRATION.md](../INTEGRATION.md) 获取更多集成方式和配置选项。
