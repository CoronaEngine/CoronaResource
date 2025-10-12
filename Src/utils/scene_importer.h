//
// Created by 25473 on 25-10-7.
//
#pragma once
#include <string>
#include <memory>
#include "scene_format.h"

struct ImportResult {
    bool success{false};
    std::string message;
    // 占位: 可加入解析后的统一 IR 指针
};

class ISceneImporter {
public:
    virtual ~ISceneImporter() = default;
    virtual SceneFormat format() const = 0;
    virtual ImportResult ImportFile(const std::string& path) = 0;
};
using ISceneImporterPtr = std::unique_ptr<ISceneImporter>;
