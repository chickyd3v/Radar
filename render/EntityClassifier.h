#pragma once

#include "data/IconTables.h"
#include "data/RadarTypes.h"
#include "sdk/PluginSDK.h"

#include <optional>

namespace RadarRender {

inline std::optional<RadarData::IconDef> LookupIcon(
    const std::unordered_map<std::string, RadarData::IconDef>& map, const char* key) {
    const auto it = map.find(key);
    if (it == map.end() || !RadarData::IsIconVisible(it->second)) return std::nullopt;
    return it->second;
}

inline std::optional<RadarData::IconDef> ClassifyEntity(const PluginSDK::Entity& e,
                                                        const RadarData::IconTables& icons) {
    using ET = PluginSDK::EntityType;
    using ES = PluginSDK::EntitySubtype;
    using EST = PluginSDK::EntityState;

    if (e.EntityState == EST::PlayerLeader) {
        if (auto d = LookupIcon(icons.baseIcons, "Leader")) return d;
    }

    switch (e.EntitySubtype) {
        case ES::PlayerSelf:
            if (auto d = LookupIcon(icons.baseIcons, "Self")) return d;
            break;
        case ES::PlayerOther:
            if (auto d = LookupIcon(icons.baseIcons, "Player")) return d;
            break;
        case ES::SpecialNPC:
            if (auto d = LookupIcon(icons.baseIcons, "Special NPC")) return d;
            break;
        case ES::POIMonster:
            if (RadarData::IsIconVisible(icons.poiMonsterDefault)) return icons.poiMonsterDefault;
            break;
        case ES::PinnacleBoss:
            if (auto d = LookupIcon(icons.baseIcons, "Pinnacle Boss Not Attackable")) return d;
            break;
        case ES::Strongbox:
            if (auto d = LookupIcon(icons.chestIcons, "Strongbox")) return d;
            break;
        case ES::JewellerStrongbox:
            if (auto d = LookupIcon(icons.chestIcons, "Jeweller's Strongbox")) return d;
            break;
        case ES::ResearcherStrongbox:
            if (auto d = LookupIcon(icons.chestIcons, "Researcher's Strongbox")) return d;
            break;
        case ES::LargeStrongbox:
            if (auto d = LookupIcon(icons.chestIcons, "Large Strongbox")) return d;
            break;
        case ES::OmenChest:
            if (auto d = LookupIcon(icons.chestIcons, "Omen Chest")) return d;
            break;
        case ES::ChestRare:
            if (auto d = LookupIcon(icons.chestIcons, "Rare Chests")) return d;
            break;
        case ES::ChestMagic:
            if (auto d = LookupIcon(icons.chestIcons, "Magic Chests")) return d;
            break;
        case ES::BreachChest:
            if (auto d = LookupIcon(icons.breachIcons, "Breach Chest")) return d;
            break;
        case ES::ExpeditionChest:
            if (auto d = LookupIcon(icons.expeditionIcons, "Generic Expedition Chests")) return d;
            break;
        default:
            break;
    }

    if (e.EntityType == ET::Monster) {
        if (e.EntityState == EST::MonsterFriendly) {
            if (auto d = LookupIcon(icons.baseIcons, "Friendly")) return d;
        }
        const char* key = "Normal Monster";
        if (e.Rarity == 1) key = "Magic Monster";
        else if (e.Rarity == 2) key = "Rare Monster";
        else if (e.Rarity >= 3) key = "Unique Monster";
        if (auto d = LookupIcon(icons.baseIcons, key)) return d;
    }
    if (e.EntityType == ET::NPC) {
        if (auto d = LookupIcon(icons.baseIcons, "NPC")) return d;
    }
    if (e.EntityType == ET::Chest) {
        if (auto d = LookupIcon(icons.chestIcons, "All Other Chest")) return d;
    }
    return std::nullopt;
}

inline ImU32 FallbackColor(const PluginSDK::Entity& e) {
    if (e.EntityType == PluginSDK::EntityType::Monster) {
        const ImVec4 colors[] = {{1.f, 0.2f, 0.2f, 1.f}, {0.4f, 0.7f, 1, 1}, {1, 1, 0.4f, 1},
                                 {1, 0.5f, 0, 1}};
        int idx = std::clamp(e.Rarity, 0, 3);
        return ImGui::ColorConvertFloat4ToU32(colors[idx]);
    }
    if (e.EntityType == PluginSDK::EntityType::NPC)
        return IM_COL32(100, 255, 100, 255);
    if (e.EntityType == PluginSDK::EntityType::Chest)
        return e.IsChestOpened ? IM_COL32(80, 128, 128, 255) : IM_COL32(0, 255, 255, 255);
    return IM_COL32(200, 200, 200, 255);
}

} // namespace RadarRender
