#pragma once

#include "EntityClassifier.h"
#include "IconAtlas.h"
#include "MapProjection.h"
#include "data/RadarConfig.h"
#include "data/TargetDatabase.h"
#include "sdk/PluginSDK.h"

#include <imgui.h>
#include <vector>

namespace RadarRender {

struct EntityDrawCache {
    std::vector<RadarData::EntityDrawCmd> cmds;
    uint64_t                              lastSnapshotTime = 0;

    void Clear() {
        cmds.clear();
        lastSnapshotTime = 0;
    }

    static bool IsSelfEntity(const PluginSDK::Entity& e, const PluginSDK::Snapshot& snap) {
        if (!snap.Player.IsValid) return false;
        if (e.EntitySubtype == PluginSDK::EntitySubtype::PlayerSelf) return true;
        if (e.Address != 0 && e.Address == snap.Player.Address) return true;
        return false;
    }

    static bool ApplyIcon(RadarData::EntityDrawCmd& cmd, const RadarData::IconDef& def) {
        if (!RadarData::IsIconVisible(def)) return false;
        cmd.useIcon = true;
        cmd.iconCx = def.cx;
        cmd.iconCy = def.cy;
        cmd.iconSize = def.scale * 0.5f;
        return true;
    }

    void Rebuild(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                 const RadarData::RadarConfig& cfg, const RadarData::TargetDatabase& db,
                 const RadarData::IconTables& icons) {
        cmds.clear();
        if (!ctx) return;
        cmds.reserve(static_cast<size_t>(std::min(cfg.MaxEntitiesDrawn, 2048)));

        auto wpath = [](const std::wstring& p) -> std::string {
            return std::string(p.begin(), p.end());
        };

        int count = 0;
        for (const auto& e : snap.Entities) {
            if (count >= cfg.MaxEntitiesDrawn) break;
            if (!e.IsValid) continue;
            if (IsSelfEntity(e, snap)) continue;
            if (e.EntityState == PluginSDK::EntityState::Useless) continue;
            if (cfg.HideOutsideNetworkBubble && e.Zone == PluginSDK::NearbyZone::Far) continue;
            if (e.EntityType == PluginSDK::EntityType::Monster && e.CurrentHP <= 0) continue;

            const std::string path = wpath(e.Path);
            if (!path.empty() && db.ignorePatterns.MatchesAny(path)) continue;

            RadarData::EntityDrawCmd cmd;
            cmd.gridX = e.GridPositionX;
            cmd.gridY = e.GridPositionY;
            cmd.terrainZ = e.TerrainHeight;

            if (auto icon = ClassifyEntity(e, icons)) ApplyIcon(cmd, *icon);

            if (!cmd.useIcon) continue;
            cmds.push_back(cmd);
            ++count;
        }

        if (snap.Player.IsValid) {
            if (auto it = icons.baseIcons.find("Self"); it != icons.baseIcons.end()) {
                RadarData::EntityDrawCmd cmd;
                cmd.gridX = snap.Player.GridPositionX;
                cmd.gridY = snap.Player.GridPositionY;
                cmd.terrainZ = snap.Player.TerrainHeight;
                if (ApplyIcon(cmd, it->second)) cmds.push_back(cmd);
            }
        }

        lastSnapshotTime = snap.LastUpdateTime;
    }

    void Draw(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap, ImDrawList* dl,
              const IconAtlas& atlas) const {
        if (!dl || !ctx || !atlas.Valid()) return;
        for (const auto& c : cmds) {
            if (!c.useIcon || c.iconSize <= 0.f) continue;
            const auto scr =
                ProjectEntityGridToScreen(ctx, snap, c.gridX, c.gridY, c.terrainZ);
            if (!scr.valid) continue;
            atlas.DrawIcon(dl, c.iconCx, c.iconCy, c.iconSize, scr.sx, scr.sy);
        }
    }
};

} // namespace RadarRender
