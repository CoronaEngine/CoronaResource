//
// Created by 25473 on 25-10-7.
//

#include "mitsuba_importer.h"
#include <fstream>

ImportResult MitsubaImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path);
    if (!ifs) { r.message = "无法打开 Mitsuba 文件"; return r; }
    // 占位: XML 解析 + 构建 IR
    r.success = true;
    r.message = "Mitsuba 解析成功(占位)";
    return r;
}