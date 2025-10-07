//
// Created by 25473 on 25-10-6.
//

#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "enums.h"
#include "unified_value.h"
#include "parameter_block.h"

struct GeometryData {
    GeometryTopology topology = GeometryTopology::TriMesh;
    ktm::fvec3 positions;
    ktm::fvec3 normals;
    ktm::fvec2 uvs;
    std::vector<uint64_t> indices;
    ParameterBlock extras;
};

struct TextureData {
    enum class Kind { Image2D, Procedural, Constant, Cube, Unknown } kind = Kind::Unknown;
    std::string source; // 文件或程序名
    ParameterBlock params;
};

struct MaterialData {
    std::string model; // 统一名称: principled, dielectric, etc.
    ParameterBlock params;
};

struct MediumData {
    ParameterBlock params; // sigma_a, sigma_s, g, etc.
};

struct LightData {
    LightKind kind = LightKind::Unknown;
    ParameterBlock params; // intensity, color, ies, etc.
};

struct CameraData {
    ParameterBlock params; // fov, sensor_size, clip, etc.
};

struct AnimationTrackData {
    // 简化: TRS keyframes
    std::vector<float> times;
    ktm::fmat4x4 local_matrices;
};
