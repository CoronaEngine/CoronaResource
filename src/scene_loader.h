//
// Created by 25473 on 25-10-7.
//
#pragma once
#include <string>
#include "utils/import_factor.h"

struct SceneLoadOutcome {
    SceneFormat format{SceneFormat::Unknown};
    bool success{false};
    std::string detectReason;
    std::string parseMessage;
};

SceneLoadOutcome LoadAnyScene(const std::string& path);
