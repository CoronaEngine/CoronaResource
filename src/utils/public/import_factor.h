//
// Created by 25473 on 25-10-7.
//
#pragma once
#include <memory>

#include <scene_format_detector.h>
#include "scene_importer.h"

ISceneImporterPtr CreateImporter(SceneFormat fmt);
ISceneImporterPtr CreateImporterForFile(const std::string& path, SceneFormatDetectResult* outDetect = nullptr);