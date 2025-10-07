//
// Created by 25473 on 25-10-1.
//

#pragma once
#include "unified_value.h"

#include <string>
#include <unordered_map>


struct ParameterBlock
{
    std::unordered_map<std::string, UnifiedValue> values;
    bool has(const std::string& k) const { return values.find(k)!=values.end(); }
    template<typename T>
    const T* get(const std::string& k) const {
        auto it = values.find(k);
        if (it==values.end()) return nullptr;
        return std::get_if<T>(&it->second);
    }
    void set(const std::string& k, UnifiedValue v, bool overwrite=true) {
        if (!overwrite && has(k)) return;
        values[k] = std::move(v);
    }
};