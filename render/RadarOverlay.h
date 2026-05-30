#pragma once

#include "IconAtlas.h"
#include "MapProjection.h"
#include "perf/AreaCache.h"
#include "data/RadarConfig.h"
#include "data/RadarLog.h"
#include "data/TargetDatabase.h"
#include "data/IconTables.h"
#include "sdk/PluginSDK.h"

#include <filesystem>
#include <imgui.h>

namespace RadarRender {

class RadarOverlay {
public:
    RadarData::RadarConfig       cfg;
    RadarData::TargetDatabase    targets;
    RadarData::IconTables        icons;
    IconAtlas                    atlas;
    RadarPerf::AreaCacheState    cache;
    PluginSDK::WalkableGridHandle walkable;
    bool                          mapWasVisible = false;

    bool ShouldDraw(const PluginSDK::Snapshot& snap) const {
        if (!cfg.OverlayEnabled) return false;
        if (!snap.IsAttached) return false;
        if (snap.State != PluginSDK::GameState::InGame) return false;
        if (cfg.DrawWhenNotInHideoutOrTown && (snap.IsTown || snap.IsHideout)) return false;
        if (cfg.DrawWhenNotPaused && snap.IsPaused) return false;
        if (cfg.HideWhenNotForeground && !snap.GameWindowForeground) return false;
        return true;
    }

    void EnsureAtlas(PluginSDK::Context* ctx, const std::filesystem::path& pluginDir) {
        if (atlas.Valid()) return;
        const auto path = pluginDir / "assets" / "icons.png";
        if (!atlas.Load(ctx->D3DDevice, path, icons.maxCx, icons.maxCy)) {
            RadarData::RadarLog::Instance().Error("Failed to load icon atlas: " + path.string());
        } else {
            RadarData::RadarLog::Instance().Info("Icon atlas loaded: " + path.string());
        }
    }

    void Draw(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap) {
        if (!ShouldDraw(snap)) return;

        auto current = ctx->Terrain.GetWalkableGrid();
        if (current.Data() != walkable.Data()) walkable = std::move(current);

        const bool mapVisible = snap.LargeMap.IsVisible || snap.MiniMap.IsVisible;
        if (mapVisible && !mapWasVisible) cache.InvalidatePoi();
        mapWasVisible = mapVisible;

        if (cache.NeedsFullRebuild(snap, walkable.Data())) {
            cache.RebuildAll(ctx, snap, walkable, cfg, targets, icons);
        } else {
            cache.RebuildPoiIfNeeded(ctx, snap, cfg, targets, icons);
            if (cache.NeedsEntityRebuild(snap))
                cache.RebuildEntitiesOnly(ctx, snap, cfg, targets, icons);
        }

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) {
            RadarData::RadarLog::Instance().Warn("DrawUI: no background draw list");
            return;
        }

        if (snap.LargeMap.IsVisible) {
            MapClipScope clip(dl, snap.LargeMap, false);
            if (cfg.DrawWalkableMap)
                cache.walkable.DrawCellsForMap(ctx, snap, dl, snap.LargeMap,
                                               ImGui::ColorConvertFloat4ToU32(cfg.WalkableMapColor),
                                               true, cache.walkable.cells);
            cache.entities.Draw(ctx, snap, dl, atlas);
            if (cfg.ShowImportantPOI) {
                cache.pois.UpdateScreenPositions(ctx, snap);
                cache.pois.Draw(dl, atlas, cfg, cfg.EdgeIndicatorLargemap, cfg.EdgeIndicatorMinimap,
                                ctx, &snap);
                cache.pois.DrawEdgeIndicators(ctx, dl, snap.LargeMap, cfg.EdgeIndicatorLargemap,
                                              false);
            }
        } else if (snap.MiniMap.IsVisible) {
            MapClipScope clip(dl, snap.MiniMap, true);
            if (cfg.DrawWalkableMap)
                cache.walkable.DrawCellsForMap(ctx, snap, dl, snap.MiniMap,
                                               ImGui::ColorConvertFloat4ToU32(cfg.WalkableMapColor),
                                               false, cache.walkable.cells);
            cache.entities.Draw(ctx, snap, dl, atlas);
            if (cfg.ShowImportantPOI) {
                cache.pois.UpdateScreenPositions(ctx, snap);
                cache.pois.Draw(dl, atlas, cfg, cfg.EdgeIndicatorLargemap, cfg.EdgeIndicatorMinimap,
                                ctx, &snap);
                cache.pois.DrawEdgeIndicators(ctx, dl, snap.MiniMap, cfg.EdgeIndicatorMinimap, true);
            }
        }
    }
};

} // namespace RadarRender
