#pragma once

#include <imgui.h>

#include <cstdint>
#include <string>

namespace RadarData {

struct Rgba8 {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    ImU32 ToImU32() const { return IM_COL32(r, g, b, a); }
};

struct IconDef {
    int   cx = 0;
    int   cy = 0;
    float scale = 30.f;
};

struct TargetEntry {
    std::string name;
    std::string path;
    bool        enabled = true;
    bool        showIcon = false;
    std::string iconName;
    float       iconSize = 30.f;
    Rgba8       nameColor{253, 224, 71, 255};
    Rgba8       bgColor{0, 0, 0, 255};
    int         expectedCount = 1;
    std::string category;
};

struct PoiResolved {
    std::string name;
    float       gridX = 0.f;
    float       gridY = 0.f;
    float       terrainZ = 0.f;
    bool        fromTgt = true;
    float       screenX = 0.f;
    float       screenY = 0.f;
    bool        hasScreen = false;
    bool        showIcon = false;
    float       iconSize = 30.f;
    int         iconCx = 1;
    int         iconCy = 37;
    Rgba8       nameColor;
    Rgba8       bgColor;
};

struct EntityDrawCmd {
    float gridX = 0.f;
    float gridY = 0.f;
    float terrainZ = 0.f;
    float screenX = 0.f;
    float screenY = 0.f;
    bool  hasScreen = false;
    int   iconCx = 0;
    int   iconCy = 0;
    float iconSize = 4.f;
    ImU32 fallbackColor = 0;
    bool  useIcon = false;
};

} // namespace RadarData
