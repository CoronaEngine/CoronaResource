#include "ResourceTypes.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace Corona {
namespace {
constexpr std::uint64_t kFnvOffset = 1469598103934665603ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

std::uint64_t fnv1a64(std::string_view text, std::uint64_t seed) {
    auto hash = seed;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= kFnvPrime;
    }
    return hash;
}

void normalize_type(ResourceType& type) {
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
}

void normalize_path(ResourcePath& path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    path.erase(std::unique(path.begin(), path.end(), [](char lhs, char rhs) {
                   return lhs == '/' && rhs == '/';
               }),
               path.end());
    if (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
}

std::uint64_t mix_uint64(std::uint64_t value, std::uint64_t seed) {
    auto hash = seed;
    for (int i = 0; i < 8; ++i) {
        const auto byte_value = static_cast<unsigned char>(value & 0xFF);
        hash ^= static_cast<std::uint64_t>(byte_value);
        hash *= kFnvPrime;
        value >>= 8;
    }
    return hash;
}
}  // namespace

std::uint64_t ResourceId::compute_uid(const ResourceType& type, const ResourcePath& path) {
    auto normalized_type = type;
    normalize_type(normalized_type);

    auto normalized_path = path;
    normalize_path(normalized_path);

    auto hash = fnv1a64(normalized_type, kFnvOffset);
    hash = fnv1a64("\n", hash);
    hash = fnv1a64(normalized_path, hash);
    return hash;
}
ResourceId ResourceId::from(ResourceType type, ResourcePath path) {
    ResourceId id;
    id.type = std::move(type);
    id.path = std::move(path);
    id.uid = compute_uid(id.type, id.path);
    return id;
}
bool ResourceId::operator==(const ResourceId& o) const noexcept {
    const auto a = uid ? uid : compute_uid(type, path);
    const auto b = o.uid ? o.uid : compute_uid(o.type, o.path);
    return a == b;
}
bool ResourceId::operator<(const ResourceId& o) const noexcept {
    const auto a = uid ? uid : compute_uid(type, path);
    const auto b = o.uid ? o.uid : compute_uid(o.type, o.path);
    return a < b;
}
std::size_t ResourceIdHash::operator()(const ResourceId& id) const noexcept {
    const auto uid_value = id.uid ? id.uid : ResourceId::compute_uid(id.type, id.path);
    return static_cast<std::size_t>(uid_value);
}
std::uint64_t SubResourceId::compute(std::uint64_t parent, SubResourceKind kind, std::uint64_t local_numeric, std::string_view local_string) {
    auto hash = mix_uint64(parent, kFnvOffset);
    hash = mix_uint64(static_cast<std::uint64_t>(kind), hash);
    hash = mix_uint64(local_numeric, hash);
    for (unsigned char ch : local_string) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= kFnvPrime;
    }
    return hash;
}
SubResourceId SubResourceId::from_index(const ResourceId& parent, SubResourceKind kind, std::uint64_t index) {
    SubResourceId s;
    s.parentUid = parent.uid ? parent.uid : ResourceId::compute_uid(parent.type, parent.path);
    s.kind = kind;
    s.uid = compute(s.parentUid, kind, index);
    return s;
}
SubResourceId SubResourceId::from_key(const ResourceId& parent, SubResourceKind kind, std::string_view key) {
    SubResourceId s;
    s.parentUid = parent.uid ? parent.uid : ResourceId::compute_uid(parent.type, parent.path);
    s.kind = kind;
    s.uid = compute(s.parentUid, kind, 0, key);
    return s;
}
SubResourceId SubResourceId::compose(std::uint64_t parent_uid, SubResourceKind kind, std::uint64_t local_numeric, std::string_view local_string) {
    SubResourceId s;
    s.parentUid = parent_uid;
    s.kind = kind;
    s.uid = compute(parent_uid, kind, local_numeric, local_string);
    return s;
}
}  // namespace Corona
