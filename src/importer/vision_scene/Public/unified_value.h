//
// Created by 25473 on 25-10-1.
//

#pragma once
#include "ktm/type_mat.h"
#include "ktm/type_vec.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

struct Reference { std::string id; };
struct ResolvedRef { uint64_t index; };
struct SpectrumStub { enum Kind { RGB, Table } kind; std::vector<float> data; };

using UnifiedScalar = double;

struct ValueArray; // 前置

using UnifiedValue = std::variant<
    std::monostate,
    bool,
    int64_t,
    UnifiedScalar,
    std::string,
    ktm::fvec2,
    ktm::fvec3,
    ktm::fmat4x4,
    SpectrumStub,
    Reference,
    ResolvedRef,
    std::shared_ptr<ValueArray>
>;

struct ValueArray {
    std::vector<UnifiedValue> items;
};