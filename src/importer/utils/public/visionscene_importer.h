//
// Created by Copilot on 25-10-16.
//
#pragma once
#include <scene_importer.h>

class VisionSceneImporter : public ISceneImporter {
public:
    SceneFormat format() const override { return SceneFormat::VisionScene; }
    ImportResult ImportFile(const std::string& path) override;
};

