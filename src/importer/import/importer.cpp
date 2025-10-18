//
// Created by Zero on 2023/7/14.
//

#include "importer.h"

#include <memory>

#include "node_desc.h"
#include "scene.h"
#include "global.h"
#include <string>
#include <filesystem>

namespace vision {

std::shared_ptr<Importer> Importer::create(const std::string &ext_name) {
    ImporterDesc desc;
    if (ext_name == ".json" || ext_name == ".bson") {
        desc.sub_type = "json";
    } else if (ext_name == ".usda" || ext_name == ".usdc") {
        desc.sub_type = "usd";
    } else {
        desc.sub_type = "assimp";
    }
    return Node::create_shared<Importer>(desc);
}

std::shared_ptr<Pipeline> Importer::import_scene(const std::filesystem::path &fn) {
    auto importer = Importer::create(fn.extension().string());
    return importer->read_file(fn);
}

}// namespace vision