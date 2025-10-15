//
// Created by 25473 on 25-10-7.
//

#include "scene_format_detector.h"
#include <fstream>
#include <cctype>
#include <algorithm>

namespace {
bool HasExt(const std::string& p, std::string_view ext) {
    if (p.size() < ext.size()) return false;
    auto tail = p.substr(p.size() - ext.size());
    std::string lowerTail(tail);
    std::transform(lowerTail.begin(), lowerTail.end(), lowerTail.begin(), ::tolower);
    std::string lowerExt(ext);
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    return lowerTail == lowerExt;
}
std::string ReadPrefix(const std::string& path, size_t n) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};
    std::string buf;
    buf.resize(n);
    ifs.read(&buf[0], std::streamsize(n));
    buf.resize(size_t(ifs.gcount()));
    return buf;
}
std::string StripCommentsAndWhitespace(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    enum { NORMAL, HASH_COMMENT } state = NORMAL;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (state == NORMAL) {
            if (c == '#') { state = HASH_COMMENT; continue; }
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            out.push_back(c);
        } else if (state == HASH_COMMENT) {
            if (c == '\n') state = NORMAL;
        }
    }
    return out;
}
bool ContainsAny(std::string_view hay, std::initializer_list<std::string_view> keys) {
    for (auto k : keys) {
        if (hay.find(k) != std::string_view::npos) return true;
    }
    return false;
}
} // namespace

SceneFormatDetectResult DetectSceneFormat(const std::string& path, size_t sniffBytes) {
    SceneFormatDetectResult r;
    std::string prefix = ReadPrefix(path, sniffBytes);
    if (prefix.empty()) {
        r.reason = "文件不可读或为空";
        return r;
    }

    // Heuristic: extension first
    bool extPbrt = HasExt(path, ".pbrt");
    bool extXml  = HasExt(path, ".xml");
    bool extDs   = HasExt(path, ".udatasmith") || HasExt(path, ".datasmith");
    bool extBlend = HasExt(path, ".blend");

    // Quick Mitsuba check (XML root)
    if (extXml || prefix.find("<scene") != std::string::npos) {
        if (ContainsAny(prefix, {"<scene", "<shape", "<sensor", "version=\"", "<emitter"})) {
            r.format = SceneFormat::Mitsuba;
            r.reason = "XML 且含 Mitsuba 典型标签";
            return r;
        }
    }

    // Datasmith check (JSON-ish or extension)
    if (extDs || (!prefix.empty() && prefix.front() == '{')) {
        if (ContainsAny(prefix, {"Datasmith", "DatasmithScene", "\"Meshes\"", "\"Actors\""})) {
            r.format = SceneFormat::Datasmith;
            r.reason = "扩展或 JSON 含 Datasmith 关键字";
            return r;
        }
    }

    if (extBlend) {
        r.format = SceneFormat::Blend;
        r.reason = "扩展名匹配 .blend";
        return r;
    }

    // PBRT: extension or directive tokens
    std::string stripped = StripCommentsAndWhitespace(prefix);
    if (extPbrt || ContainsAny(stripped, {
        "WorldBegin","Film","Camera","Integrator","MakeNamedMaterial","Shape","LightSource","Material"
    })) {
        r.format = SceneFormat::PBRT;
        r.reason = extPbrt ? "扩展名匹配 .pbrt" : "出现 PBRT 指令关键字";
        return r;
    }

    r.reason = "未匹配任何已知格式特征";
    return r;
}