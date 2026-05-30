#pragma once

#include "IconAtlas.h"
#include "MapProjection.h"
#include "data/PathMatcher.h"
#include "data/RadarConfig.h"
#include "data/RadarLog.h"
#include "data/TargetDatabase.h"
#include "data/IconTables.h"
#include "sdk/PluginSDK.h"

#include <cmath>
#include <imgui.h>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace RadarRender {

struct PoiDrawCache {
    std::vector<RadarData::PoiResolved> pois;
    bool                                textBackground = true;
    uint64_t                            lastDrawLogTime = 0;

    static RadarData::IconDef ResolvePoiIcon(const RadarData::TargetEntry& t,
                                             const RadarData::IconTables& icons) {
        if (!t.iconName.empty()) {
            const auto findIn = [&](const auto& m) -> std::optional<RadarData::IconDef> {
                if (auto it = m.find(t.iconName); it != m.end()) return it->second;
                return std::nullopt;
            };
            if (auto d = findIn(icons.baseIcons)) return *d;
            if (auto d = findIn(icons.chestIcons)) return *d;
            if (auto d = findIn(icons.breachIcons)) return *d;
            if (auto d = findIn(icons.deliriumIcons)) return *d;
            if (auto d = findIn(icons.expeditionIcons)) return *d;
        }
        return icons.otherImportantDefault;
    }

    void Clear() { pois.clear(); }

    void UpdateScreenPositions(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap) {
        for (auto& p : pois) {
            p.hasScreen = false;
            ProjectedScreen scr;
            if (p.fromTgt) {
                if (snap.LargeMap.IsVisible) {
                    scr = ProjectTgtToLargeMapScreen(ctx, snap, p.gridX, p.gridY);
                    if (scr.valid) {
                        p.screenX = scr.sx;
                        p.screenY = scr.sy;
                        p.hasScreen = true;
                        continue;
                    }
                }
                if (snap.MiniMap.IsVisible) {
                    scr = ProjectTgtToMiniMapScreen(ctx, snap, p.gridX, p.gridY);
                    if (scr.valid) {
                        p.screenX = scr.sx;
                        p.screenY = scr.sy;
                        p.hasScreen = true;
                        if (snap.Player.IsValid) {
                            const auto ps = ProjectTgtToMiniMapScreen(
                                ctx, snap, snap.Player.GridPositionX, snap.Player.GridPositionY);
                            if (ps.valid) {
                                const float dx = p.screenX - ps.sx;
                                const float dy = p.screenY - ps.sy;
                                if ((dx * dx + dy * dy) < (14.f * 14.f)) p.hasScreen = false;
                            }
                        }
                    }
                }
            } else {
                if (snap.LargeMap.IsVisible) {
                    scr = ProjectGridToLargeMapScreen(ctx, snap, p.gridX, p.gridY, p.terrainZ);
                    if (scr.valid) {
                        p.screenX = scr.sx;
                        p.screenY = scr.sy;
                        p.hasScreen = true;
                        continue;
                    }
                }
                if (snap.MiniMap.IsVisible) {
                    scr = ProjectGridToMiniMapScreen(ctx, snap, p.gridX, p.gridY, p.terrainZ);
                    if (scr.valid) {
                        p.screenX = scr.sx;
                        p.screenY = scr.sy;
                        p.hasScreen = true;
                    }
                }
            }
        }
    }

    void Rebuild(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                 const RadarData::RadarConfig& cfg, const RadarData::TargetDatabase& db,
                 const RadarData::IconTables& icons) {
        Clear();
        if (!ctx || !cfg.ShowImportantPOI) return;
        textBackground = cfg.EnablePOIBackground;

        const std::string areaKey =
            db.ResolveAreaKey(snap.CurrentAreaHash, snap.CurrentAreaName);
        const auto targets = db.GetTargetsForArea(snap.CurrentAreaHash, snap.CurrentAreaName);
        std::ostringstream log;
        log << "POI rebuild hash='" << snap.CurrentAreaHash << "' name='" << snap.CurrentAreaName
            << "' key='" << areaKey << "' targets=" << targets.size()
            << " largeMap=" << snap.LargeMap.IsVisible;

        if (targets.empty()) {
            RadarData::RadarLog::Instance().Warn(log.str() + "' — no targets for area");
            return;
        }

        std::vector<RadarData::CompiledPattern> compiled;
        compiled.reserve(targets.size());
        for (const auto* t : targets) compiled.push_back(RadarData::CompilePattern(t->path));

        struct TgtCand {
            float gx = 0.f;
            float gy = 0.f;
        };
        std::vector<std::vector<TgtCand>> perTarget(targets.size());

        int tgtEnum = 0;
        std::string tgtSamples;
        ctx->Terrain.EnumerateTgtLocations([&](const PluginSDK::TgtLocation& loc) {
            if (tgtEnum < 2) {
                if (!tgtSamples.empty()) tgtSamples += " | ";
                tgtSamples += loc.Path;
            }
            ++tgtEnum;
            for (size_t i = 0; i < targets.size(); ++i) {
                if (!RadarData::MatchPattern(compiled[i], loc.Path)) continue;
                perTarget[i].push_back({loc.X, loc.Y});
            }
            return true;
        });

        int tgtHits = 0;
        constexpr float kMinPoiGridSep = 80.f;

        for (size_t i = 0; i < targets.size(); ++i) {
            if (perTarget[i].empty()) continue;
            const auto* t = targets[i];

            float centX = 0.f, centY = 0.f;
            for (const TgtCand& c : perTarget[i]) {
                centX += c.gx;
                centY += c.gy;
            }
            const float invN = 1.f / static_cast<float>(perTarget[i].size());
            centX *= invN;
            centY *= invN;

            float bestGx = 0.f, bestGy = 0.f;
            float bestScore = -1e30f;
            for (const TgtCand& c : perTarget[i]) {
                bool tooClose = false;
                for (const auto& placed : pois) {
                    if (std::hypot(c.gx - placed.gridX, c.gy - placed.gridY) < kMinPoiGridSep) {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) continue;

                float score = -std::hypot(c.gx - centX, c.gy - centY);
                if (ctx->Terrain.IsWalkable(static_cast<int>(c.gx), static_cast<int>(c.gy)))
                    score += 500.f;
                if (snap.LargeMap.IsVisible || snap.MiniMap.IsVisible) {
                    if (ProjectTgtToMapScreen(ctx, snap, c.gx, c.gy).valid) score += 5000.f;
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestGx = c.gx;
                    bestGy = c.gy;
                }
            }
            if (bestScore < -1e20f) continue;

            RadarData::PoiResolved p;
            p.name = t->name;
            p.gridX = bestGx;
            p.gridY = bestGy;
            p.terrainZ = 0.f;
            p.fromTgt = true;
            p.showIcon = t->showIcon;
            p.iconSize = t->iconSize > 0 ? t->iconSize : 30.f;
            const auto iconDef = ResolvePoiIcon(*t, icons);
            p.iconCx = iconDef.cx;
            p.iconCy = iconDef.cy;
            p.nameColor = t->nameColor;
            p.bgColor = t->bgColor;
            pois.push_back(std::move(p));
            ++tgtHits;
        }

        auto wpath = [](const std::wstring& p) { return std::string(p.begin(), p.end()); };
        int entHits = 0;
        for (const auto& e : snap.Entities) {
            if (!e.IsValid) continue;
            const std::string path = wpath(e.Path);
            if (path.empty()) continue;
            for (size_t i = 0; i < targets.size(); ++i) {
                if (!RadarData::MatchPattern(compiled[i], path)) continue;
                const auto* t = targets[i];
                RadarData::PoiResolved p;
                p.name = t->name;
                p.gridX = e.GridPositionX;
                p.gridY = e.GridPositionY;
                p.terrainZ = e.TerrainHeight;
                p.fromTgt = false;
                p.showIcon = t->showIcon;
                p.iconSize = t->iconSize > 0 ? t->iconSize : 30.f;
                const auto iconDef = ResolvePoiIcon(*t, icons);
                p.iconCx = iconDef.cx;
                p.iconCy = iconDef.cy;
                p.nameColor = t->nameColor;
                p.bgColor = t->bgColor;
                pois.push_back(std::move(p));
                ++entHits;
            }
        }

        log << " resolved=" << pois.size() << " tgtHits=" << tgtHits << " entHits=" << entHits
            << " tgtEnum=" << tgtEnum;
        if (!pois.empty()) {
            log << " firstPoi='" << pois.front().name << "' grid=(" << pois.front().gridX << ","
                << pois.front().gridY << ")";
        }
        if (tgtHits == 0 && !tgtSamples.empty()) log << " tgtSample='" << tgtSamples << "'";
        RadarData::RadarLog::Instance().Info(log.str());
    }

    void Draw(ImDrawList* dl, const IconAtlas& atlas, const RadarData::RadarConfig& cfg,
              bool edgeLarge, bool edgeMini, PluginSDK::Context* ctx = nullptr,
              const PluginSDK::Snapshot* snap = nullptr) {
        if (!dl) return;
        (void)cfg;
        int visible = 0;
        int iconDrawn = 0;
        int dotDrawn = 0;
        for (const auto& p : pois) {
            if (!p.hasScreen) continue;
            ++visible;
            const ImU32 nameCol = p.nameColor.ToImU32();

            if (cfg.DrawPoiIcons && p.showIcon && atlas.Valid()) {
                atlas.DrawIcon(dl, p.iconCx, p.iconCy, p.iconSize, p.screenX, p.screenY, nameCol);
                ++iconDrawn;
            } else {
                dl->AddCircleFilled(ImVec2(p.screenX, p.screenY), 4.f, nameCol);
                ++dotDrawn;
            }

            const char* label = p.name.c_str();
            ImVec2 ts = ImGui::CalcTextSize(label);
            ImVec2 pos(p.screenX - ts.x * 0.5f, p.screenY - ts.y - 8.f);
            if (textBackground)
                dl->AddRectFilled(ImVec2(pos.x - 2, pos.y - 1),
                                  ImVec2(pos.x + ts.x + 2, pos.y + ts.y + 1),
                                  p.bgColor.ToImU32());
            dl->AddLine(ImVec2(p.screenX, p.screenY), ImVec2(pos.x + ts.x * 0.5f, pos.y + ts.y),
                        IM_COL32(255, 255, 255, 180), 1.f);
            dl->AddText(pos, nameCol, label);
        }
        (void)edgeLarge;
        (void)edgeMini;

        if (ctx && snap && !pois.empty()
            && snap->LastUpdateTime - lastDrawLogTime > 8000) {
            lastDrawLogTime = snap->LastUpdateTime;
            if (visible == 0) {
                RadarData::RadarLog::Instance().Warn(
                    "POI draw: " + std::to_string(pois.size())
                    + " cached, 0 on map (open large map; TGT uses GridToLargeMap z=0)");
            } else {
                RadarData::RadarLog::Instance().Info(
                    "POI draw: " + std::to_string(visible) + "/" + std::to_string(pois.size())
                    + " on map, icons=" + std::to_string(iconDrawn)
                    + " dots=" + std::to_string(dotDrawn)
                    + " (dots are terrain POI, not monsters)");
            }
        }
    }

    void DrawEdgeIndicators(PluginSDK::Context* ctx, ImDrawList* dl,
                            const PluginSDK::MapData& map, bool enabled,
                            bool minimap) const {
        if (!dl || !enabled || !map.IsVisible) return;
        const float margin = 8.f;

        for (const auto& p : pois) {
            if (!p.hasScreen) continue;
            if (IsInsideMapViewport(ctx, map, p.screenX, p.screenY, minimap)) continue;

            float sx = p.screenX;
            float sy = p.screenY;
            if (minimap) {
                const float radius = std::min(map.SizeX, map.SizeY) * 0.5f - margin;
                if (radius > 1.f) {
                    float clipCx = map.CenterX, clipCy = map.CenterY;
                    GetMinimapClipOrigin(ctx, map, clipCx, clipCy);
                    const float dx = sx - clipCx;
                    const float dy = sy - clipCy;
                    const float len = std::sqrt(dx * dx + dy * dy);
                    if (len > 0.001f) {
                        sx = clipCx + dx * (radius / len);
                        sy = clipCy + dy * (radius / len);
                    }
                }
            } else {
                const float halfW = map.SizeX * 0.5f;
                const float halfH = map.SizeY * 0.5f;
                const float minX = map.CenterX - halfW + margin;
                const float maxX = map.CenterX + halfW - margin;
                const float minY = map.CenterY - halfH + margin;
                const float maxY = map.CenterY + halfH - margin;
                sx = std::clamp(sx, minX, maxX);
                sy = std::clamp(sy, minY, maxY);
            }
            dl->AddCircleFilled(ImVec2(sx, sy), 5.f, p.nameColor.ToImU32());
        }
    }
};

} // namespace RadarRender
