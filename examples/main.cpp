//
// Created by 25473 on 25-9-29.
//

#include <filesystem>
#include <iostream>
#include <algorithm>

#include "CoronaResourceVersion.h"
#include "scene_loader.h"

// Resource Manager includes
#include <Model.h>
#include <ResourceTypes.h>


#ifndef CORONARESOURCE_SOURCE_DIR
#define CORONARESOURCE_SOURCE_DIR ""
#endif

void resourceManagerExample() {
    std::cout << "\n=== Resource Manager Example ===\n";
    
    // 创建 ModelLoader 实例
    Corona::ModelLoader modelLoader;
    
    // 测试路径 - 使用项目中的一个模型文件路径
    std::filesystem::path modelPath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_model";
    
    // 查找场景目录中的模型文件
    std::vector<std::string> modelExtensions = {".obj", ".fbx", ".gltf", ".glb", ".dae"};
    std::vector<std::string> modelFiles;
    std::string foundModelPath;
    
    if (std::filesystem::exists(modelPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(modelPath)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (std::find(modelExtensions.begin(), modelExtensions.end(), ext) != modelExtensions.end()) {
                    foundModelPath = entry.path().string();
                    modelFiles.emplace_back(foundModelPath);
                }
            }
        }
    }
    
    if (modelFiles.empty()) {
        std::cout << "未找到测试模型文件，跳过 Resource Manager 示例\n";
        return;
    }

    for (const auto& f : modelFiles) {
        std::cout << "可用模型文件: " << f << "\n";
        // 创建 ResourceId
        Corona::ResourceId modelId = Corona::ResourceId::from("model", f);

        // 检查是否支持该资源类型
        if (!modelLoader.supports(modelId)) {
            std::cout << "ModelLoader 不支持该资源类型\n";
            return;
        }

        std::cout << "开始加载模型...\n";

        // 加载模型
        auto resource = modelLoader.load(modelId);

        if (!resource) {
            std::cout << "模型加载失败\n";
            return;
        }

        if (auto model = std::dynamic_pointer_cast<Corona::Model>(resource)) {
            std::cout << "模型加载成功！\n";
            std::cout << "  - 网格数量: " << model->meshes.size() << "\n";
            std::cout << "  - 骨骼动画数量: " << model->skeletalAnimations.size() << "\n";
            std::cout << "  - 骨骼数量: " << model->m_BoneCounter << "\n";
            std::cout << "  - 包围盒最小点: ("
                      << model->minXYZ.x << ", "
                      << model->minXYZ.y << ", "
                      << model->minXYZ.z << ")\n";
            std::cout << "  - 包围盒最大点: ("
                      << model->maxXYZ.x << ", "
                      << model->maxXYZ.y << ", "
                      << model->maxXYZ.z << ")\n";

            // 显示每个网格的信息
            for (size_t i = 0; i < model->meshes.size(); ++i) {
                const auto& mesh = model->meshes[i];
                std::cout << "  - 网格 #" << i << ":\n";
                std::cout << "    顶点数: " << mesh.points.size() / 3 << "\n";
                std::cout << "    索引数: " << mesh.Indices.size() << "\n";
            }
        } else {
            std::cout << "无法转换为 Model 类型\n";
        }

    }

    std::cout << "=== Resource Manager Example 结束 ===\n\n";
}

int main(int /*argc*/, char** /*argv*/) {
    // 运行 Resource Manager 示例

    // resourceManagerExample();

    // 固定内部测试场景路径（使用源目录 + 示例测试场景）
    // std::filesystem::path scenePath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_pbrt" / "kitchen" / "scene-v4.pbrt";
    std::filesystem::path scenePath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_mitsuba" / "kitchen" / "scene_v0.6.xml";
    // std::filesystem::path scenePath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_datasmith" / "test.udatasmith";
    // std::filesystem::path scenePath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_scenes" / "cbox" / "cbox.blend";
    // std::filesystem::path scenePath = std::filesystem::path(CORONARESOURCE_SOURCE_DIR) / "Examples" / "test_tunsten" / "kitchen" / "scene.json";

    std::cout << "CORONARESOURCEExample (Version: " << corona_resource::VersionString << ")\n";
    std::cout << "使用固定内部场景文件: " << scenePath.string() << "\n";

    if (!std::filesystem::exists(scenePath)) {
        std::cerr << "内部场景文件不存在 " << scenePath.string() << "\n";
        return 3;
    }

    auto outcome = LoadAnyScene(scenePath.string());
    std::cout << "检测格式: " << ToString(outcome.format) << "\n";
    std::cout << "检测理由: " << outcome.detectReason << "\n";
    std::cout << "解析结果: " << (outcome.success ? "成功" : "失败") << "\n";
    std::cout << "解析信息: " << outcome.parseMessage << "\n";

    return outcome.success ? 0 : 2;
}