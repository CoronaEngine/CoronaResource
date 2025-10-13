#pragma once

#include <cstdint>
#include <ktm/ktm.h>
#include <memory>
#include <vector>


namespace Corona
{
    class Model;
    class Animation;

    // 运行时动画数据：选择哪个动画、当前时间、输出骨矩阵
    struct AnimationState
    {
        std::shared_ptr<Model> model;    // 目标模型（包含骨骼映射和动画列表）
        uint32_t animationIndex = 0;     // 当前播放的动画索引
        float currentTime = 0.0f;        // 当前播放的时间（秒）
        std::vector<ktm::fmat4x4> bones; // 输出：骨矩阵（大小与 BoneInfo 数量一致）
    };
} // namespace Corona
