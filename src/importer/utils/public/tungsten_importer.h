//
// Created by 25473 on 25-10-15.
//
#pragma once
#include <scene_importer.h>

class TungstenImporter : public ISceneImporter {
public:
    SceneFormat format() const override { return SceneFormat::Tungsten; }
    ImportResult ImportFile(const std::string& path) override;
};