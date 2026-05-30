#pragma once

#include "IconTables.h"
#include "RadarConfig.h"
#include "RadarLog.h"
#include "TargetDatabase.h"

#include <filesystem>

namespace RadarData {

inline void ApplyDefaultSettings(RadarConfig& cfg) {
    cfg = RadarConfig{};
}

inline void ApplyDefaultIcons(IconTables& icons) {
    icons = IconTables{};
    icons.SeedDefaults();
}

inline bool ResetAllToDefaults(const std::filesystem::path& pluginDir,
                               RadarConfig& cfg, IconTables& icons,
                               TargetDatabase& targets) {
    ApplyDefaultSettings(cfg);
    ApplyDefaultIcons(icons);
    cfg.Save(pluginDir);
    icons.Save(pluginDir);

    std::error_code ec;
    std::filesystem::remove(pluginDir / "config" / "targets" / "user.json", ec);
    targets.Load(pluginDir);
    RadarLog::Instance().Info("Reset to defaults: settings + icons reloaded");
    return true;
}

} // namespace RadarData
