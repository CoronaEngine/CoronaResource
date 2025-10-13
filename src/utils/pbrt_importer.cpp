//
// Created by 25473 on 25-10-7.
//

#include "pbrt_importer.h"
#include <fstream>

ImportResult PBRTImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path);
    if (!ifs) { r.message = "无法打开 PBRT 文件"; return r; }
    // 占位: 在这里调用实际 PBRT 解析流程
    r.success = true;
    r.message = "PBRT 解析成功(占位)";
    return r;
}