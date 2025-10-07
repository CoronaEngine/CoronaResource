//
// Created by 25473 on 25-10-1.
//

#pragma once

#include <string>
#include <cstdint>

struct SourceLocation {
    uint32_t file_id = 0;
    uint32_t line = 0;
    uint32_t col = 0;
    std::string original_id;
};