//
// Created by 25473 on 2025/10/15.
//

#pragma once
#include "scene_importer.h"

class BlendImporter : public ISceneImporter {
public:
    SceneFormat format() const override { return SceneFormat::Blend; }
    ImportResult ImportFile(const std::string& path) override;
};