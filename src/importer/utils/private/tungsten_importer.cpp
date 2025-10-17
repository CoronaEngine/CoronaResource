//
// Created by 25473 on 25-10-15.
//
#include "../public/tungsten_importer.h"

#include <fstream>

ImportResult TungstenImporter::ImportFile(const std::string& path) {
    ImportResult r;
    std::ifstream ifs(path);
    if (!ifs) { r.message = "无法打开 Tungsten 场景 JSON"; return r; }
    // 占位: 读取 JSON，解析 camera/integrator/bsdfs/primitives 等
    r.success = true;
    r.message = "Tungsten 解析成功(占位)";
    return r;
}


