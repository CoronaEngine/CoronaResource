//
// Created by 25473 on 25-10-1.
//

#pragma once
#include <cstdint>


enum class NodeKind : uint8_t {
    Geometry,
    Material,
    Texture,
    Light,
    Camera,
    Medium,
    Instance,
    Environment,
    Volume,
    AnimationTrack,
    Group,
    Unknown
};

enum class GeometryTopology : uint8_t {
    TriMesh,
    Curve,
    QuadMesh,
    PointCloud,
    Procedural
};

enum class LightKind : uint8_t {
    Point,
    Spot,
    Directional,
    Area,
    Environment,
    Distant,
    Unknown
};