#pragma once

#include "MapProjection.h"
#include "WalkableGridView.h"
#include "data/RadarConfig.h"
#include "sdk/PluginSDK.h"

#include <imgui.h>
#include <vector>

namespace RadarRender {

struct WalkableBake {
    struct Cell {
        uint16_t gx = 0;
        uint16_t gy = 0;
        float    z = 0.f;
    };

    std::vector<Cell> cells;
    ImU32             color = 0;
    bool              valid = false;

    void Clear() {
        cells.clear();
        valid = false;
    }

    void Rebuild(PluginSDK::Context* ctx, const PluginSDK::WalkableGridHandle& grid,
                 const RadarData::RadarConfig& cfg) {
        Clear();
        if (!grid.Valid() || !ctx) return;

        const uint8_t* data = grid.Data();
        const int w = grid.Width();
        const int h = grid.Height();
        const size_t sizeBytes = grid.SizeBytes();
        if (!data || w <= 0 || h <= 0) return;

        color = ImGui::ColorConvertFloat4ToU32(cfg.WalkableMapColor);
        int step = std::max(2, cfg.WalkableDecimation);
        while ((w / step) * (h / step) > 15000 && step < 32) step *= 2;

        cells.reserve(static_cast<size_t>((w / step) * (h / step) / 3));
        for (int gy = 0; gy < h; gy += step) {
            for (int gx = 0; gx < w; gx += step) {
                if (!IsWalkableCell(data, sizeBytes, w, h, gx, gy)) continue;
                Cell c;
                c.gx = static_cast<uint16_t>(gx);
                c.gy = static_cast<uint16_t>(gy);
                c.z = ctx->Terrain.GetTerrainHeight(gx, gy);
                cells.push_back(c);
            }
        }
        valid = !cells.empty();
    }

    static void DrawCellsForMap(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                                ImDrawList* dl, const PluginSDK::MapData& map, ImU32 color,
                                bool useLargeMap, const std::vector<Cell>& cells) {
        if (!map.IsVisible) return;
        for (const Cell& c : cells) {
            ProjectedScreen scr;
            if (useLargeMap) {
                scr = ProjectGridToLargeMapScreen(ctx, snap, static_cast<float>(c.gx),
                                                  static_cast<float>(c.gy), c.z);
            } else {
                scr = ProjectGridToMiniMapScreen(ctx, snap, static_cast<float>(c.gx),
                                                 static_cast<float>(c.gy), c.z, 12.f, false);
            }
            if (!scr.valid) continue;
            if (useLargeMap) {
                if (!IsInsideMapRect(map, scr.sx, scr.sy)) continue;
            } else if (!IsInsideMinimapDisc(map, scr.sx, scr.sy, 12.f)) {
                continue;
            }
            dl->AddRectFilled(ImVec2(scr.sx, scr.sy), ImVec2(scr.sx + 1.f, scr.sy + 1.f),
                              color);
        }
    }

    void Draw(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap, ImDrawList* dl,
              const PluginSDK::MapData& largeMap, const PluginSDK::MapData& miniMap) const {
        if (!dl || !valid || !ctx) return;
        if (largeMap.IsVisible)
            DrawCellsForMap(ctx, snap, dl, largeMap, color, true, cells);
        if (miniMap.IsVisible)
            DrawCellsForMap(ctx, snap, dl, miniMap, color, false, cells);
    }
};

} // namespace RadarRender
