//
// Created by 25473 on 25-10-7.
//

#include "datasmith_importer.h"
#include <fstream>

ImportResult DatasmithImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path);
    if (!ifs) { r.message = "无法打开 Datasmith 文件"; return r; }
    // 占位: JSON / 压缩容器解析
    r.success = true;
    r.message = "Datasmith 解析成功(占位)";
    return r;
}