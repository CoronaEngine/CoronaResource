#pragma once
// Moved from Src/Core/IO/ResourceTypes.h (same content, comments trimmed)
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Corona {
using ResourceType = std::string;
using ResourcePath = std::string;
struct ResourceId {
    ResourceType type;
    ResourcePath path;
    std::uint64_t uid{0};
    static std::uint64_t compute_uid(const ResourceType& type, const ResourcePath& path);
    static ResourceId from(ResourceType type, ResourcePath path);
    bool operator==(const ResourceId& o) const noexcept;
    bool operator<(const ResourceId& o) const noexcept;
};
struct ResourceIdHash {
    std::size_t operator()(const ResourceId& id) const noexcept;
};
template <typename T>
using ResourcePtr = std::shared_ptr<T>;
enum class SubResourceKind : std::uint32_t {
    Unknown = 0,
    Geometry = 1,
    GeometryInstance,
    Material,
    Texture,
    Node,
    Animation,
    Other = 255
};
struct SubResourceId {
    std::uint64_t uid{0};
    std::uint64_t parentUid{0};
    SubResourceKind kind{SubResourceKind::Unknown};
    static std::uint64_t compute(std::uint64_t parent, SubResourceKind kind, std::uint64_t local_numeric, std::string_view local_string = {});
    static SubResourceId from_index(const ResourceId& parent, SubResourceKind kind, std::uint64_t index);
    static SubResourceId from_key(const ResourceId& parent, SubResourceKind kind, std::string_view key);
    static SubResourceId compose(std::uint64_t parent_uid, SubResourceKind kind, std::uint64_t local_numeric, std::string_view local_string = {});
};
}  // namespace Corona
