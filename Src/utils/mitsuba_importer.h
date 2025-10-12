//
// Created by 25473 on 25-10-7.
//

#pragma once
#include "scene_importer.h"

class MitsubaImporter : public ISceneImporter {
public:
    SceneFormat format() const override { return SceneFormat::Mitsuba; }
    ImportResult ImportFile(const std::string& path) override;
};
