//
// Created by 25473 on 25-10-6.
//

#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "scene_node.h"
#include "resource_data.h"

struct Scene {
    std::vector<SceneNode> nodes;

    std::vector<GeometryData> geometries;
    std::vector<MaterialData> materials;
    std::vector<TextureData> textures;
    std::vector<LightData> lights;
    std::vector<CameraData> cameras;
    std::vector<MediumData> media;
    std::vector<AnimationTrackData> animations;

    std::unordered_map<std::string, uint32_t> id_to_node;

    uint64_t add_node(const SceneNode& n) {
        uint64_t idx = (uint64_t)nodes.size();
        nodes.push_back(n);
        if (!n.id.empty()) id_to_node[n.id] = idx;
        return idx;
    }
};
