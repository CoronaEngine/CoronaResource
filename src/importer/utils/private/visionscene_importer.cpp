//
// Created by Copilot on 25-10-16.
//
#include <visionscene_importer.h>

#include <fstream>
#include <string>

static bool LooksLikeVisionSceneJsonPrefix(const std::string& prefix) {
    // Accept explicit marker anywhere in the prefix window
    if (prefix.find("\"corona_resource\"") != std::string::npos) return true;
    return false;
}

ImportResult VisionSceneImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) { r.message = "无法打开 VisionScene 场景 JSON"; return r; }
    std::string prefix; prefix.resize(4096);
    ifs.read(&prefix[0], std::streamsize(prefix.size()));
    prefix.resize(size_t(ifs.gcount()));
    if (!LooksLikeVisionSceneJsonPrefix(prefix)) {
        r.message = "文件不是有效的 VisionScene JSON 外观";
        return r;
    }
    // TODO: integrate real parser to fill unified Scene IR
    r.success = true;
    r.message = "VisionScene 解析成功(占位)";
    return r;
}
