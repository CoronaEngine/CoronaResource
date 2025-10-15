//
// Created by 25473 on 25-10-7.
//

#pragma once
#include <string>

enum class SceneFormat {
    PBRT,
    Mitsuba,
    Datasmith,
    Blend,
    Tungsten,
    Unknown
};

inline const char* ToString(SceneFormat f) {
    switch (f) {
        case SceneFormat::PBRT: return "PBRT";
        case SceneFormat::Mitsuba: return "Mitsuba";
        case SceneFormat::Datasmith: return "Datasmith";
        case SceneFormat::Blend: return "Blend";
        case SceneFormat::Tungsten: return "Tungsten";
        case SceneFormat::Unknown: return "Unknown";
    }
}