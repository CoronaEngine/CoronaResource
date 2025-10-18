//
// Created by 25473 on 25-10-1.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ktm/ktm.h"
#include "enums.h"
#include "parameter_block.h"
#include "source_location.h"

struct SceneNode {
    NodeKind kind = NodeKind::Unknown;
    std::string id;               // 可为空
    uint32_t parent = UINT32_MAX; // 根为 MAX
    std::vector<uint32_t> children;
    ktm::fmat4x4 local_transform {};   // 规范空间
    ParameterBlock params;        // 节点级参数（引用资源 / override）
    SourceLocation src;
    // 资源索引引用（可不适用于所有 kind）
    int32_t geometry_index = -1;
    int32_t material_index = -1;
    int32_t texture_index  = -1;  // 主纹理或绑定
    int32_t medium_index   = -1;
    int32_t light_index    = -1;
    int32_t camera_index   = -1;
    int32_t animation_index = -1;
};
