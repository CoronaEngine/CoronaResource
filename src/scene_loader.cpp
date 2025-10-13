//
// Created by 25473 on 25-10-7.
//

#include "scene_loader.h"

SceneLoadOutcome LoadAnyScene(const std::string& path) {
    SceneLoadOutcome o;
    SceneFormatDetectResult det;
    auto importer = CreateImporterForFile(path, &det);
    o.format = det.format;
    o.detectReason = det.reason;
    if (!importer) {
        o.parseMessage = "无法识别的场景格式";
        return o;
        }
    auto res = importer->ImportFile(path);
    o.success = res.success;
    o.parseMessage = res.message;
    return o;
}