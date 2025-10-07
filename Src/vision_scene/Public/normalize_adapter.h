//
// Created by 25473 on 25-10-6.
//

#pragma once
#include "unified_value.h"
#include "scene.h"

// 输入一个来源空间矩阵, 转为内部 canonical(示例右手 Y_up)
ktm::fmat4x4 NormalizeTransformFromSource(const ktm::fmat4x4& src, const std::string& sourceTag);
