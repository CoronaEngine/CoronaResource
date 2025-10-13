//
// Created by 25473 on 25-9-29.
//

#include <iostream>
#include <filesystem>
#include "scene_loader.h"
#include "VisionSceneVersion.h"

#ifndef VISIONSCENE_SOURCE_DIR
#define VISIONSCENE_SOURCE_DIR ""
#endif

int main(int /*argc*/, char** /*argv*/) {
    // 固定内部测试场景路径（使用源目录 + 示例测试场景）
    std::filesystem::path scenePath = std::filesystem::path(VISIONSCENE_SOURCE_DIR) / "Examples" / "test_scenes" / "sample.pbrt";

    std::cout << "VisionSceneExample (Version: " << vision_scene::VersionString << ")\n";
    std::cout << "使用固定内部场景文件: " << scenePath.string() << "\n";

    if (!std::filesystem::exists(scenePath)) {
        std::cerr << "内部场景文件不存在，请确认路径或将 sample.pbrt 放置于: " << scenePath.string() << "\n";
        return 3;
    }

    auto outcome = LoadAnyScene(scenePath.string());
    std::cout << "检测格式: " << ToString(outcome.format) << "\n";
    std::cout << "检测理由: " << outcome.detectReason << "\n";
    std::cout << "解析结果: " << (outcome.success ? "成功" : "失败") << "\n";
    std::cout << "解析信息: " << outcome.parseMessage << "\n";


    return outcome.success ? 0 : 2;
}