//
// Created by 25473 on 2025/10/15.
//

#include "../public/blend_importer.h"

#include <fstream>

ImportResult BlendImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path);
    if (!ifs) { r.message = "无法打开 Blend 文件"; return r; }
    // 占位: JSON / 压缩容器解析
    r.success = true;
    r.message = "Blend 解析成功(占位)";
    return r;
}