#pragma once

#include "sdk/PluginSDK.h"

#include <imgui.h>

#include <cmath>

namespace RadarRender {

struct ProjectedScreen {
    float sx = 0.f;
    float sy = 0.f;
    bool  valid = false;
};

inline float WorldToGridScale(const PluginSDK::Snapshot& snap, PluginSDK::Context* ctx) {
    if (snap.WorldToGridConvertor > 1.f) return snap.WorldToGridConvertor;
    if (ctx) {
        const float w = ctx->Terrain.GetWorldToGridConvertor();
        if (w > 1.f) return w;
    }
    return 10.25f;
}

inline bool ProjectWithMapTransform(const PluginSDK::MapTransform& t, float gx, float gy,
                                    float worldZ, float worldToGrid, float& sx, float& sy) {
    if (!t.IsVisible) return false;
    const float dx = gx - t.PlayerGridX;
    const float dy = gy - t.PlayerGridY;
    sx = t.CenterX + (dx - dy) * t.ScaleX;
    sy = t.CenterY + (worldZ * worldToGrid - (dx + dy)) * t.ScaleY;
    return true;
}

inline bool IsInsideMapRect(const PluginSDK::MapData& map, float sx, float sy,
                            float margin = 2.f) {
    if (!map.IsVisible || map.SizeX <= 0.f || map.SizeY <= 0.f) return false;
    const float halfW = map.SizeX * 0.5f;
    const float halfH = map.SizeY * 0.5f;
    return sx >= map.CenterX - halfW + margin && sx <= map.CenterX + halfW - margin
           && sy >= map.CenterY - halfH + margin && sy <= map.CenterY + halfH - margin;
}

inline float MinimapClipRadius(const PluginSDK::MapData& map, float inset = 10.f) {
    if (!map.IsVisible) return 0.f;
    return std::max(1.f, std::min(map.SizeX, map.SizeY) * 0.5f - inset);
}

inline bool IsInsideMinimapDisc(const PluginSDK::MapData& map, float sx, float sy,
                                float inset = 10.f) {
    if (!map.IsVisible) return false;
    const float radius = MinimapClipRadius(map, inset);
    if (radius <= 1.f) return IsInsideMapRect(map, sx, sy, inset);
    const float dx = sx - map.CenterX;
    const float dy = sy - map.CenterY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

inline bool IsInsideMapViewport(const PluginSDK::MapData& map, float sx, float sy,
                                bool minimap) {
    return minimap ? IsInsideMinimapDisc(map, sx, sy) : IsInsideMapRect(map, sx, sy);
}

inline void PushLargeMapClipRect(ImDrawList* dl, const PluginSDK::MapData& map) {
    if (!dl || !map.IsVisible) return;
    const float halfW = map.SizeX * 0.5f;
    const float halfH = map.SizeY * 0.5f;
    dl->PushClipRect(ImVec2(map.CenterX - halfW, map.CenterY - halfH),
                     ImVec2(map.CenterX + halfW, map.CenterY + halfH), true);
}

struct MapClipScope {
    ImDrawList*                 dl = nullptr;
    bool                        pushed = false;

    // Minimap is circular in-game; use per-primitive disc tests only (no square clip rect).
    MapClipScope(ImDrawList* drawList, const PluginSDK::MapData& map, bool minimap) : dl(drawList) {
        if (dl && map.IsVisible && !minimap) {
            PushLargeMapClipRect(dl, map);
            pushed = true;
        }
    }

    ~MapClipScope() {
        if (pushed && dl) dl->PopClipRect();
    }

    MapClipScope(const MapClipScope&) = delete;
    MapClipScope& operator=(const MapClipScope&) = delete;
};

inline ProjectedScreen ProjectGridToLargeMapScreen(PluginSDK::Context* ctx,
                                                   const PluginSDK::Snapshot& snap,
                                                   float gx, float gy, float worldZ) {
    ProjectedScreen out;
    if (!ctx || !snap.LargeMap.IsVisible) return out;

    if (ctx->Render.GridToLargeMap(gx, gy, worldZ, out.sx, out.sy)
        && IsInsideMapRect(snap.LargeMap, out.sx, out.sy)) {
        out.valid = true;
        return out;
    }

    const auto t = ctx->Render.GetLargeMapTransform();
    const float wtg = WorldToGridScale(snap, ctx);
    float sx = 0.f, sy = 0.f;
    if (ProjectWithMapTransform(t, gx, gy, worldZ, wtg, sx, sy)
        && IsInsideMapRect(snap.LargeMap, sx, sy)) {
        out.sx = sx;
        out.sy = sy;
        out.valid = true;
    }
    return out;
}

inline ProjectedScreen ProjectGridToMiniMapScreen(PluginSDK::Context* ctx,
                                                  const PluginSDK::Snapshot& snap,
                                                  float gx, float gy, float worldZ,
                                                  float discInset = 10.f,
                                                  bool allowTransformFallback = true) {
    ProjectedScreen out;
    if (!ctx || !snap.MiniMap.IsVisible) return out;

    if (ctx->Render.GridToMiniMap(gx, gy, worldZ, out.sx, out.sy)
        && IsInsideMinimapDisc(snap.MiniMap, out.sx, out.sy, discInset)) {
        out.valid = true;
        return out;
    }

    if (!allowTransformFallback) return out;

    const auto t = ctx->Render.GetMiniMapTransform();
    const float wtg = WorldToGridScale(snap, ctx);
    float sx = 0.f, sy = 0.f;
    if (ProjectWithMapTransform(t, gx, gy, worldZ, wtg, sx, sy)
        && IsInsideMinimapDisc(snap.MiniMap, sx, sy, discInset)) {
        out.sx = sx;
        out.sy = sy;
        out.valid = true;
    }
    return out;
}

// TGT / POI: SDK docs require worldZ = 0. Do not use MapTransform fallback (wrong cluster).
inline ProjectedScreen ProjectTgtToLargeMapScreen(PluginSDK::Context* ctx,
                                                  const PluginSDK::Snapshot& snap,
                                                  float gx, float gy) {
    ProjectedScreen out;
    if (!ctx || !snap.LargeMap.IsVisible) return out;
    if (ctx->Render.GridToLargeMap(gx, gy, 0.f, out.sx, out.sy)
        && IsInsideMapRect(snap.LargeMap, out.sx, out.sy)) {
        out.valid = true;
    }
    return out;
}

inline ProjectedScreen ProjectTgtToMiniMapScreen(PluginSDK::Context* ctx,
                                                 const PluginSDK::Snapshot& snap,
                                                 float gx, float gy) {
    ProjectedScreen out;
    if (!ctx || !snap.MiniMap.IsVisible) return out;
    if (ctx->Render.GridToMiniMap(gx, gy, 0.f, out.sx, out.sy)
        && IsInsideMinimapDisc(snap.MiniMap, out.sx, out.sy)) {
        out.valid = true;
    }
    return out;
}

inline ProjectedScreen ProjectTgtToMapScreen(PluginSDK::Context* ctx,
                                             const PluginSDK::Snapshot& snap,
                                             float gx, float gy) {
    if (snap.LargeMap.IsVisible) {
        const auto out = ProjectTgtToLargeMapScreen(ctx, snap, gx, gy);
        if (out.valid) return out;
    }
    return ProjectTgtToMiniMapScreen(ctx, snap, gx, gy);
}

inline ProjectedScreen ProjectGridToMapScreen(PluginSDK::Context* ctx,
                                              const PluginSDK::Snapshot& snap,
                                              float gx, float gy, float terrainZ) {
    if (snap.LargeMap.IsVisible) {
        const auto out = ProjectGridToLargeMapScreen(ctx, snap, gx, gy, terrainZ);
        if (out.valid) return out;
    }
    return ProjectGridToMiniMapScreen(ctx, snap, gx, gy, terrainZ);
}

inline ProjectedScreen ProjectGridToScreen(PluginSDK::Context* ctx,
                                           const PluginSDK::Snapshot& snap,
                                           float gx, float gy, float terrainZ) {
    ProjectedScreen out;
    if (!ctx) return out;

    if (snap.LargeMap.IsVisible || snap.MiniMap.IsVisible)
        return ProjectGridToMapScreen(ctx, snap, gx, gy, terrainZ);

    if (snap.Player.IsValid) {
        const float conv = WorldToGridScale(snap, ctx);
        const float wx =
            snap.Player.WorldX + (gx - snap.Player.GridPositionX) * conv;
        const float wy =
            snap.Player.WorldY + (gy - snap.Player.GridPositionY) * conv;
        if (ctx->Render.WorldToScreen(wx, wy, terrainZ, out.sx, out.sy)) {
            out.valid = true;
            return out;
        }
    }

    return out;
}

inline ProjectedScreen ProjectEntityGridToScreen(PluginSDK::Context* ctx,
                                                 const PluginSDK::Snapshot& snap,
                                                 float gx, float gy, float terrainZ) {
    if (snap.LargeMap.IsVisible)
        return ProjectGridToLargeMapScreen(ctx, snap, gx, gy, terrainZ);
    if (snap.MiniMap.IsVisible)
        return ProjectGridToMiniMapScreen(ctx, snap, gx, gy, terrainZ, 8.f, true);
    return {};
}

inline ProjectedScreen ProjectEntityToScreen(PluginSDK::Context* ctx,
                                             const PluginSDK::Snapshot& snap,
                                             const PluginSDK::Entity& e) {
    ProjectedScreen out;
    if (!ctx || !e.IsValid) return out;

    if (snap.LargeMap.IsVisible || snap.MiniMap.IsVisible)
        return ProjectEntityGridToScreen(ctx, snap, e.GridPositionX, e.GridPositionY,
                                         e.TerrainHeight);

    if (ctx->Render.WorldToScreen(e.WorldX, e.WorldY, e.WorldZ, out.sx, out.sy))
        out.valid = true;
    return out;
}

} // namespace RadarRender
