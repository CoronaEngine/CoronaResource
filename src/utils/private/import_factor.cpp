//
// Created by 25473 on 25-10-7.
//

#include "../public/import_factor.h"

#include "../public/pbrt_importer.h"
#include "../public/blend_importer.h"
#include "../public/datasmith_importer.h"
#include "../public/mitsuba_importer.h"
#include "../public/tungsten_importer.h"

ISceneImporterPtr CreateImporter(SceneFormat fmt) {
    switch (fmt) {
        case SceneFormat::PBRT: return std::make_unique<PBRTImporter>();
        case SceneFormat::Mitsuba: return std::make_unique<MitsubaImporter>();
        case SceneFormat::Datasmith: return std::make_unique<DatasmithImporter>();
        case SceneFormat::Blend: return std::make_unique<BlendImporter>();
        case SceneFormat::Tungsten: return std::make_unique<TungstenImporter>();
        default: return {};
    }
}

ISceneImporterPtr CreateImporterForFile(const std::string& path, SceneFormatDetectResult* outDetect) {
    auto det = DetectSceneFormat(path);
    if (outDetect) *outDetect = det;
    if (det.format == SceneFormat::Unknown) return {};
    return CreateImporter(det.format);
}
