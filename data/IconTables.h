#pragma once

#include "RadarTypes.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "../third_party/json.hpp"

namespace RadarData {

inline bool IsIconVisible(const IconDef& d) { return d.scale > 0.f; }

struct IconTables {
    std::unordered_map<std::string, IconDef> baseIcons;
    std::unordered_map<std::string, IconDef> chestIcons;
    std::unordered_map<std::string, IconDef> breachIcons;
    std::unordered_map<std::string, IconDef> deliriumIcons;
    std::unordered_map<std::string, IconDef> expeditionIcons;
    IconDef poiMonsterDefault{12, 44, 30};
    IconDef otherImportantDefault{1, 37, 30};
    int maxCx = 14;
    int maxCy = 71;

    static IconDef ParseIconDef(const nlohmann::json& o) {
        return IconDef{static_cast<int>(o.value("CX", 0.f)),
                       static_cast<int>(o.value("CY", 0.f)),
                       o.value("Scale", 30.f)};
    }

    static void LoadMap(const nlohmann::json& j,
                        std::unordered_map<std::string, IconDef>& out,
                        int& maxCx, int& maxCy) {
        if (!j.is_object()) return;
        for (auto it = j.begin(); it != j.end(); ++it) {
            IconDef d = ParseIconDef(it.value());
            out[it.key()] = d;
            maxCx = std::max(maxCx, d.cx + 1);
            maxCy = std::max(maxCy, d.cy + 1);
        }
    }

    void Load(const std::filesystem::path& pluginDir) {
        baseIcons.clear();
        chestIcons.clear();
        breachIcons.clear();
        deliriumIcons.clear();
        expeditionIcons.clear();
        const auto path = pluginDir / "config" / "icons.json";
        if (!std::filesystem::exists(path)) {
            SeedDefaults();
            return;
        }
        std::ifstream in(path);
        if (!in.is_open()) {
            SeedDefaults();
            return;
        }
        nlohmann::json j;
        in >> j;
        LoadMap(j.value("BaseIcons", nlohmann::json::object()), baseIcons, maxCx, maxCy);
        LoadMap(j.value("ChestIcons", nlohmann::json::object()), chestIcons, maxCx, maxCy);
        LoadMap(j.value("BreachIcons", nlohmann::json::object()), breachIcons, maxCx, maxCy);
        LoadMap(j.value("DeliriumIcons", nlohmann::json::object()), deliriumIcons, maxCx, maxCy);
        LoadMap(j.value("ExpeditionIcons", nlohmann::json::object()), expeditionIcons, maxCx, maxCy);
        if (j.contains("POIMonsters"))
            for (auto& [k, v] : j["POIMonsters"].items()) {
                poiMonsterDefault = ParseIconDef(v);
                break;
            }
        if (j.contains("OtherImportantObjects"))
            for (auto& [k, v] : j["OtherImportantObjects"].items()) {
                otherImportantDefault = ParseIconDef(v);
                break;
            }
    }

    void Save(const std::filesystem::path& pluginDir) const {
        auto writeMap = [](const std::unordered_map<std::string, IconDef>& m) {
            nlohmann::json o = nlohmann::json::object();
            for (const auto& [k, d] : m)
                o[k] = {{"CX", d.cx}, {"CY", d.cy}, {"Scale", d.scale}};
            return o;
        };
        nlohmann::json j;
        j["BaseIcons"] = writeMap(baseIcons);
        j["ChestIcons"] = writeMap(chestIcons);
        j["BreachIcons"] = writeMap(breachIcons);
        j["DeliriumIcons"] = writeMap(deliriumIcons);
        j["ExpeditionIcons"] = writeMap(expeditionIcons);
        j["POIMonsters"] = {{"-1", {{"CX", poiMonsterDefault.cx}, {"CY", poiMonsterDefault.cy},
                                      {"Scale", poiMonsterDefault.scale}}}};
        j["OtherImportantObjects"] = {
            {"-1", {{"CX", otherImportantDefault.cx}, {"CY", otherImportantDefault.cy},
                    {"Scale", otherImportantDefault.scale}}}};
        const auto path = pluginDir / "config" / "icons.json";
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path);
        if (out.is_open()) out << j.dump(4);
    }

    void SeedDefaults() {
        auto put = [](auto& m, const char* n, int cx, int cy, float sc) { m[n] = IconDef{cx, cy, sc}; };
        put(baseIcons, "Self", 0, 0, 15);
        put(baseIcons, "Player", 2, 0, 15);
        put(baseIcons, "Leader", 3, 1, 25);
        put(baseIcons, "Friendly", 1, 0, 15);
        put(baseIcons, "NPC", 3, 0, 15);
        put(baseIcons, "Special NPC", 13, 42, 30);
        put(baseIcons, "Normal Monster", 0, 14, 30);
        put(baseIcons, "Magic Monster", 6, 3, 30);
        put(baseIcons, "Rare Monster", 4, 57, 30);
        put(baseIcons, "Unique Monster", 6, 57, 30);
        put(baseIcons, "Pinnacle Boss Not Attackable", 6, 6, 30);
        put(baseIcons, "Shrine", 7, 0, 30);
        put(chestIcons, "Strongbox", 1, 58, 30);
        put(chestIcons, "Jeweller's Strongbox", 11, 60, 35);
        put(chestIcons, "Researcher's Strongbox", 6, 62, 35);
        put(chestIcons, "Large Strongbox", 8, 62, 35);
        put(chestIcons, "Omen Chest", 3, 58, 40);
        put(chestIcons, "Rare Chests", 1, 70, 30);
        put(chestIcons, "Magic Chests", 2, 70, 25);
        put(chestIcons, "All Other Chest", 10, 10, 15);
        put(breachIcons, "Breach Chest", 1, 2, 20);
        put(deliriumIcons, "Delirium Bomb", 11, 37, 15);
        put(deliriumIcons, "Delirium Spawner", 1, 40, 15);
        put(expeditionIcons, "Cavern Entrance", 0, 2, 50);
        put(expeditionIcons, "Chest Quantity Remnant", 11, 40, 60);
        put(expeditionIcons, "Logbook", 4, 40, 50);
        put(expeditionIcons, "Splinter Chest", 4, 40, 55);
        maxCx = 14;
        maxCy = 71;
    }
};

} // namespace RadarData
