#pragma once

#include "data/RadarConfig.h"
#include "data/RadarDefaults.h"
#include "data/IconTables.h"
#include "data/PathMatcher.h"
#include "data/TargetDatabase.h"
#include "render/IconAtlas.h"
#include "render/RadarOverlay.h"

#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <unordered_set>

namespace RadarUi {

struct UiState {
    char        iconSearch[128]{};
    bool        iconPickerOpen = false;
    std::string iconPickerTarget;
    std::unordered_map<std::string, RadarData::IconDef>* iconPickerMap = nullptr;
    float       setAllIconSize = 30.f;

    bool        pickerPoiMode = false;
    bool        pickerEntityMode = false;
    bool        requestResetDefaults = false;

    std::unordered_set<std::string> expandedAreas;

    bool        editModalOpen = false;
    RadarData::TargetEntry editTarget;
    std::string editAreaKey;
    bool        editIsNew = false;
};

inline void DrawIconPicker(UiState& ui, RadarRender::IconAtlas& atlas, RadarData::IconTables& icons) {
    if (!ui.iconPickerOpen || !ui.iconPickerMap) return;
    ImGui::SetNextWindowSize(ImVec2(400, 340), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Icon Picker", &ui.iconPickerOpen)) {
        ImGui::End();
        return;
    }
    ImGui::InputTextWithHint("##search", "Search icons by name...", ui.iconSearch,
                             IM_ARRAYSIZE(ui.iconSearch));
    ImGui::Separator();
    const int cols = atlas.Valid() ? atlas.GridCols() : icons.maxCx;
    const int rows = atlas.Valid() ? std::min(8, atlas.GridRows()) : 8;
    ImGui::BeginChild("##grid", ImVec2(0, -4), true);
    for (int cy = 0; cy < rows; ++cy) {
        for (int cx = 0; cx < cols; ++cx) {
            ImGui::PushID(cy * cols + cx);
            if (atlas.Valid()) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                atlas.DrawIcon(ImGui::GetWindowDrawList(), cx, cy, 22.f, p.x + 12.f, p.y + 12.f);
                if (ImGui::InvisibleButton("##cell", ImVec2(26, 26)) && ui.iconPickerMap
                    && !ui.iconPickerTarget.empty()) {
                    if (auto it = ui.iconPickerMap->find(ui.iconPickerTarget);
                        it != ui.iconPickerMap->end()) {
                        it->second.cx = cx;
                        it->second.cy = cy;
                    }
                    ui.iconPickerOpen = false;
                }
            } else if (ImGui::Button("##b", ImVec2(24, 24))) {
                if (auto it = ui.iconPickerMap->find(ui.iconPickerTarget);
                    it != ui.iconPickerMap->end()) {
                    it->second.cx = cx;
                    it->second.cy = cy;
                }
                ui.iconPickerOpen = false;
            }
            if (cx + 1 < cols) ImGui::SameLine();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

inline void DrawCompactIconGroup(const char* title,
                                 std::unordered_map<std::string, RadarData::IconDef>& map,
                                 UiState& ui, RadarRender::IconAtlas& atlas) {
    if (!ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 2.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 1.f));
    if (ImGui::BeginTable(title, 4,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg
                              | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Pre", ImGuiTableColumnFlags_WidthFixed, 28.f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 34.f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 88.f);
        ImGui::TableHeadersRow();

        for (auto& [name, def] : map) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 22.f);
            ImGui::TableSetColumnIndex(0);
            if (atlas.Valid()) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                atlas.DrawIcon(ImGui::GetWindowDrawList(), def.cx, def.cy, 18.f, p.x + 12.f,
                               p.y + 11.f);
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(name.c_str());
            if (ImGui::Button("##pick", ImVec2(28.f, 18.f))) {
                ui.iconPickerOpen = true;
                ui.iconPickerTarget = name;
                ui.iconPickerMap = &map;
            }
            if (atlas.Valid()) {
                ImVec2 p = ImGui::GetItemRectMin();
                atlas.DrawIcon(ImGui::GetWindowDrawList(), def.cx, def.cy, 16.f, p.x + 14.f,
                               p.y + 9.f);
            }
            ImGui::TableSetColumnIndex(3);
            if (ImGui::SmallButton("-")) def.scale = (def.scale > 1.f) ? def.scale - 1.f : 0.f;
            ImGui::SameLine(0, 2);
            ImGui::SetNextItemWidth(36.f);
            ImGui::DragFloat("##sz", &def.scale, 0.5f, 0.f, 80.f, "%.0f");
            ImGui::SameLine(0, 2);
            if (ImGui::SmallButton("+")) def.scale += 1.f;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar(2);
}

inline void DrawGeneralTab(RadarData::RadarConfig& cfg) {
    bool en = cfg.OverlayEnabled;
    if (ImGui::Checkbox("Enable Radar Overlay", &en)) cfg.OverlayEnabled = en;
    ImGui::SameLine();
    ImGui::TextColored(en ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(0.6f, 0.6f, 0.6f, 1.f),
                       en ? "(Enabled)" : "(Disabled)");
    ImGui::Separator();
    ImGui::Checkbox("Terrain Overlay (Maphack)", &cfg.DrawWalkableMap);
    ImGui::SameLine();
    ImGui::ColorEdit4("##wcolor", &cfg.WalkableMapColor.x,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::DragInt("Border", &cfg.WalkableMapBorderThickness, 1, 0, 8);
    ImGui::DragInt("Walkable decimation", &cfg.WalkableDecimation, 1, 2, 16);
    ImGui::Checkbox("Hide in Towns/Hideouts", &cfg.DrawWhenNotInHideoutOrTown);
    ImGui::Checkbox("Hide on Pause", &cfg.DrawWhenNotPaused);
    ImGui::Checkbox("Hide when alt-tabbed", &cfg.HideWhenNotForeground);
    ImGui::Checkbox("POI icon sprites on map", &cfg.DrawPoiIcons);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Large map icons for POI targets. Off by default (labels + small dots only). "
            "Icons mark terrain TGT tiles, not visible monsters.");
    ImGui::Checkbox("Hide Distant Entities (network bubble)", &cfg.HideOutsideNetworkBubble);
    ImGui::Checkbox("Draw Player Names", &cfg.ShowPlayerNames);
    ImGui::DragInt("Max entities drawn", &cfg.MaxEntitiesDrawn, 16, 64, 2048);
}

inline void DrawIconsTab(RadarData::RadarConfig& cfg, RadarData::IconTables& icons, UiState& ui,
                         RadarRender::IconAtlas& atlas) {
    ImGui::Checkbox("Edge Indicators (Minimap)", &cfg.EdgeIndicatorMinimap);
    ImGui::SameLine();
    ImGui::Checkbox("Edge Indicators (Large Map)", &cfg.EdgeIndicatorLargemap);
    ImGui::TextWrapped("Customize icon size and type for different entity categories.");
    ImGui::SetNextItemWidth(60.f);
    ImGui::InputFloat("Set All Size", &ui.setAllIconSize, 0, 0, "%.0f");
    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        auto apply = [&](auto& m) {
            for (auto& [k, d] : m) d.scale = ui.setAllIconSize;
        };
        apply(icons.baseIcons);
        apply(icons.chestIcons);
        apply(icons.breachIcons);
        apply(icons.deliriumIcons);
        apply(icons.expeditionIcons);
    }
    ImGui::BeginChild("##iconScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    DrawCompactIconGroup("Base Icons", icons.baseIcons, ui, atlas);
    DrawCompactIconGroup("Chest Icons", icons.chestIcons, ui, atlas);
    DrawCompactIconGroup("Delirium Icons", icons.deliriumIcons, ui, atlas);
    DrawCompactIconGroup("Breach Icons", icons.breachIcons, ui, atlas);
    DrawCompactIconGroup("Expedition Icons", icons.expeditionIcons, ui, atlas);
    ImGui::EndChild();
}

inline void PushPoiActionButtonStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.32f, 0.58f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.40f, 0.68f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.28f, 0.50f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
}

inline void PopPoiActionButtonStyle() { ImGui::PopStyleColor(4); }

inline bool DrawPoiCategoryHeader(const char* id, const char* label, bool defaultOpen) {
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.10f, 0.28f, 0.52f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.14f, 0.34f, 0.60f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.08f, 0.24f, 0.46f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
                               | ImGuiTreeNodeFlags_AllowOverlap;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    const bool open = ImGui::TreeNodeEx(label, flags);
    ImGui::PopStyleColor(4);
    ImGui::PopID();
    return open;
}

inline void DrawTargetIndicesTable(RadarData::TargetDatabase& db,
                                   const std::vector<size_t>& indices,
                                   const std::string& areaKey, UiState& ui,
                                   RadarRender::RadarOverlay& overlay,
                                   RadarRender::IconAtlas& atlas) {
    if (indices.empty()) return;

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.f, 3.f));
    ImGui::PushID(areaKey.c_str());
    const ImGuiTableFlags tblFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg
                                     | ImGuiTableFlags_SizingStretchProp
                                     | ImGuiTableFlags_ScrollX;
    if (!ImGui::BeginTable("poi", 8, tblFlags)) {
        ImGui::PopID();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.f);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 140.f);
    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 2.f);
    ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 36.f);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 36.f);
    ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 40.f);
    ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed, 26.f);
    ImGui::TableSetupColumn("Del", ImGuiTableColumnFlags_WidthFixed, 26.f);
    ImGui::TableHeadersRow();

    for (size_t idx : indices) {
        if (idx >= db.storage.size()) continue;
        auto& t = db.storage[idx];
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Checkbox(("##en" + std::to_string(idx)).c_str(), &t.enabled);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(t.name.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(t.path.c_str());
        ImGui::TableSetColumnIndex(3);
        if (t.showIcon && atlas.Valid()) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            atlas.DrawIcon(ImGui::GetWindowDrawList(), 1, 37, 18.f, p.x + 12.f, p.y + 10.f);
        }
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.0f", t.iconSize);
        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%d", t.expectedCount);
        ImGui::TableSetColumnIndex(6);
        ImGui::PushID(static_cast<int>(idx * 2));
        PushPoiActionButtonStyle();
        if (ImGui::Button("E", ImVec2(22.f, 20.f))) {
            ui.editTarget = t;
            ui.editAreaKey = areaKey;
            ui.editIsNew = false;
            ui.editModalOpen = true;
        }
        PopPoiActionButtonStyle();
        ImGui::PopID();
        ImGui::TableSetColumnIndex(7);
        ImGui::PushID(static_cast<int>(idx * 2 + 1));
        PushPoiActionButtonStyle();
        if (ImGui::Button("X", ImVec2(22.f, 20.f))) {
            t.enabled = false;
            overlay.cache.InvalidatePoi();
        }
        PopPoiActionButtonStyle();
        ImGui::PopID();
    }
    ImGui::EndTable();
    ImGui::PopID();
    ImGui::PopStyleVar();
}

inline void DrawAreaTargetTable(RadarData::TargetDatabase& db, const std::string& areaKey,
                                UiState& ui, RadarRender::RadarOverlay& overlay,
                                RadarRender::IconAtlas& atlas) {
    const std::string key = RadarData::NormalizeAreaKey(areaKey);
    if (key == "*" || key == "GLOBAL") {
        DrawTargetIndicesTable(db, db.actsGlobalTargets, key, ui, overlay, atlas);
        return;
    }
    auto it = db.byArea.find(key);
    if (it == db.byArea.end()) return;
    DrawTargetIndicesTable(db, it->second, key, ui, overlay, atlas);
}

inline bool DrawAreaSubNode(RadarData::TargetDatabase& db, const std::string& areaKey,
                            const std::string& label, bool isCurrent, bool defaultOpen,
                            UiState& ui, RadarRender::RadarOverlay& overlay) {
    ImGui::PushID(areaKey.c_str());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (isCurrent) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.92f, 0.35f, 1.f));
    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    if (isCurrent) ImGui::PopStyleColor();
    if (open) {
        DrawAreaTargetTable(db, areaKey, ui, overlay, overlay.atlas);
        ImGui::TreePop();
    }
    ImGui::PopID();
    return open;
}

inline std::string ResolveCurrentAreaKey(const PluginSDK::Snapshot& snap,
                                         const RadarData::TargetDatabase& db) {
    return db.ResolveAreaKey(snap.CurrentAreaHash, snap.CurrentAreaName);
}

inline void DrawAreaTreeSection(RadarData::TargetDatabase& db, const std::string& sourceLabel,
                                const std::string& sourceKey, const std::string& currentArea,
                                UiState& ui, RadarRender::RadarOverlay& overlay) {
    const auto areas = db.ListAreas(sourceKey);
    const bool hasGlobal = !db.actsGlobalTargets.empty() && sourceKey == "Acts";
    if (areas.empty() && !hasGlobal && sourceKey != "Endgame") return;

    const bool sectionOpen =
        DrawPoiCategoryHeader(sourceKey.c_str(), sourceLabel.c_str(), sourceKey == "Acts");
    if (!sectionOpen) return;

    if (hasGlobal && sourceKey == "Acts") {
        DrawAreaSubNode(db, "*", "Global", false, true, ui, overlay);
    }

    if (!currentArea.empty()) {
        const bool inThisSource =
            db.areaSource.count(currentArea)
            && db.areaSource.at(currentArea) == sourceKey;
        if (inThisSource) {
            std::string label = db.DisplayNameForArea(currentArea) + " [Current Location]";
            DrawAreaSubNode(db, currentArea, label, true, true, ui, overlay);
        }
    }

    for (const std::string& area : areas) {
        if (!currentArea.empty() && RadarData::AreaKeysEqual(area, currentArea)) continue;
        const std::string label = db.DisplayNameForArea(area);
        DrawAreaSubNode(db, area, label, false, false, ui, overlay);
    }

    ImGui::TreePop();
}

inline void DrawEditTargetModal(UiState& ui, RadarData::TargetDatabase& db,
                                const std::filesystem::path& pluginDir,
                                RadarRender::RadarOverlay& overlay) {
    if (!ui.editModalOpen) return;
    ImGui::SetNextWindowSize(ImVec2(480, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Edit Target", &ui.editModalOpen)) {
        ImGui::End();
        return;
    }
    char nameBuf[256];
    strncpy_s(nameBuf, ui.editTarget.name.c_str(), sizeof(nameBuf) - 1);
    ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
    ui.editTarget.name = nameBuf;
    char pathBuf[512];
    strncpy_s(pathBuf, ui.editTarget.path.c_str(), sizeof(pathBuf) - 1);
    ImGui::InputText("Metadata Path", pathBuf, sizeof(pathBuf));
    ui.editTarget.path = pathBuf;
    ImGui::Checkbox("Enabled", &ui.editTarget.enabled);
    ImGui::DragInt("Count", &ui.editTarget.expectedCount, 1, 1, 99);
    ImGui::Checkbox("Show as Icon", &ui.editTarget.showIcon);
    if (ui.editTarget.showIcon) ImGui::DragFloat("Icon Size", &ui.editTarget.iconSize, 1, 5, 80);
    if (ImGui::Button("Save", ImVec2(90, 0))) {
        if (ui.editIsNew) {
            db.AddUserTarget(ui.editAreaKey, ui.editTarget);
            db.SaveUser(pluginDir);
        } else {
            for (auto& t : db.storage) {
                if (t.name == ui.editTarget.name && t.path == ui.editTarget.path) {
                    t = ui.editTarget;
                    break;
                }
            }
        }
        overlay.cache.InvalidatePoi();
        ui.editModalOpen = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(90, 0))) ui.editModalOpen = false;
    ImGui::End();
}

inline void DrawObjectsTab(RadarData::RadarConfig& cfg, RadarData::TargetDatabase& db,
                           UiState& ui, const PluginSDK::Snapshot& snap,
                           const std::filesystem::path& pluginDir,
                           RadarRender::RadarOverlay& overlay) {
    ImGui::BeginDisabled(true);
    bool pathFinder = false;
    ImGui::Checkbox("Enable PathFinder", &pathFinder);
    ImGui::EndDisabled();
    ImGui::SameLine(0.f, 24.f);
    bool multicolor = false;
    ImGui::BeginDisabled(true);
    ImGui::Checkbox("Multicolor", &multicolor);
    ImGui::EndDisabled();

    ImGui::Checkbox("Points of Interest (POI)", &cfg.ShowImportantPOI);
    ImGui::SameLine(0.f, 24.f);
    ImGui::Checkbox("Text Background", &cfg.EnablePOIBackground);

    if (ImGui::Button("Add POI from Map", ImVec2(0, 0))) {
        ui.pickerPoiMode = true;
        ui.pickerEntityMode = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Entity from Map", ImVec2(0, 0))) {
        ui.pickerEntityMode = true;
        ui.pickerPoiMode = false;
    }

    const std::string currentArea = ResolveCurrentAreaKey(snap, db);

    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 4.f));
    ImGui::BeginChild("##poiTree", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    DrawAreaTreeSection(db, "Endgame", "Endgame", currentArea, ui, overlay);
    DrawAreaTreeSection(db, "Acts (Level < 65)", "Acts", currentArea, ui, overlay);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    DrawEditTargetModal(ui, db, pluginDir, overlay);
}

inline void DrawSettings(RadarRender::RadarOverlay& overlay, UiState& ui,
                          const PluginSDK::Snapshot& snap,
                          const std::filesystem::path& pluginDir) {
    if (ImGui::Button("Reset to Default")) ui.requestResetDefaults = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(settings, icons — not custom POI targets)");

    if (ImGui::BeginTabBar("RadarTabs")) {
        if (ImGui::BeginTabItem("General Settings")) {
            DrawGeneralTab(overlay.cfg);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Icon Management")) {
            DrawIconsTab(overlay.cfg, overlay.icons, ui, overlay.atlas);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Objects")) {
            DrawObjectsTab(overlay.cfg, overlay.targets, ui, snap, pluginDir, overlay);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    DrawIconPicker(ui, overlay.atlas, overlay.icons);
}

} // namespace RadarUi
