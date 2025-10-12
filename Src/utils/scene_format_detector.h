//
// Created by 25473 on 25-10-7.
//

#pragma once
#include <string>
#include <string_view>
#include "scene_format.h"

struct SceneFormatDetectResult {
    SceneFormat format{SceneFormat::Unknown};
    std::string reason;
};

SceneFormatDetectResult DetectSceneFormat(const std::string& path, size_t sniffBytes = 8192);