// Official Radar plugin — SDK v6, performance-first overlay.

#include "sdk/PluginSDK.h"

#include "data/Migration.h"
#include "data/RadarDefaults.h"
#include "data/RadarLog.h"
#include "render/RadarOverlay.h"
#include "ui/RadarUi.h"

#include <imgui.h>

class RadarPlugin : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Radar"; }

    bool WantsOverlay() const override { return m_overlay.cfg.OverlayEnabled; }

    void OnEnable(bool /*isGameAttached*/) override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        const auto pluginDir = DirectoryPath();
        RadarData::RadarLog::Instance().Init(pluginDir);

        const auto hostDir = pluginDir.parent_path().parent_path();
        RadarData::TryMigrateFromHost(pluginDir, hostDir);

        m_overlay.cfg.Load(pluginDir);
        m_overlay.icons.Load(pluginDir);
        m_overlay.targets.Load(pluginDir);
        if (RadarData::TargetDatabase::SyncBundledTargetsFromHost(pluginDir, hostDir, true,
                                                                &m_overlay.targets))
            m_overlay.cache.InvalidatePoi();
        m_overlay.walkable = ctx()->Terrain.GetWalkableGrid();
        m_overlay.EnsureAtlas(const_cast<PluginSDK::Context*>(ctx()), pluginDir);

        RadarData::RadarLog::Instance().Info("Radar plugin enabled");
        ctx()->Log.Info("Radar plugin enabled — see logs/radar.log in plugin folder");
    }

    void OnDisable() override {
        EndPickerMode();
        m_overlay.walkable.Reset();
        m_overlay.atlas.Release();
        m_overlay.cache.Clear();
        RadarData::RadarLog::Instance().Info("Radar plugin disabled");
        RadarData::RadarLog::Instance().Shutdown();
        ctx()->Log.Info("Radar plugin disabled");
    }

    void DrawSettings() override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        const auto pluginDir = DirectoryPath();
        if (m_ui.requestResetDefaults) {
            m_ui.requestResetDefaults = false;
            std::error_code ec;
            std::filesystem::remove(pluginDir / "config" / "targets" / "user.json", ec);
            RadarData::ResetAllToDefaults(pluginDir, m_overlay.cfg, m_overlay.icons,
                                          m_overlay.targets);
            m_overlay.cache.Clear();
            m_overlay.cache.InvalidatePoi();
            RadarData::RadarLog::Instance().Info("Settings reset to defaults");
            ctx()->Log.Info("Radar settings reset to defaults");
        }

        const auto snap = ctx()->Game.GetSnapshot();
        m_overlay.EnsureAtlas(const_cast<PluginSDK::Context*>(ctx()), pluginDir);
        RadarUi::DrawSettings(m_overlay, m_ui, snap, pluginDir);
    }

    void DrawUI() override {
        if (!m_overlay.cfg.OverlayEnabled) return;
        if (!ctx()->Game.IsInGame()) return;
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        const auto snap = ctx()->Game.GetSnapshot();

        if (m_ui.pickerPoiMode || m_ui.pickerEntityMode) {
            DrawPicker(snap);
            return;
        }

        m_overlay.EnsureAtlas(const_cast<PluginSDK::Context*>(ctx()), DirectoryPath());
        m_overlay.Draw(const_cast<PluginSDK::Context*>(ctx()), snap);
    }

    void SaveSettings() override {
        const auto dir = DirectoryPath();
        m_overlay.cfg.Save(dir);
        m_overlay.icons.Save(dir);
        m_overlay.targets.SaveUser(dir);
    }

private:
    RadarRender::RadarOverlay m_overlay;
    RadarUi::UiState          m_ui;

    void EndPickerMode() {
        if (!m_ui.pickerPoiMode && !m_ui.pickerEntityMode) return;
        m_ui.pickerPoiMode = false;
        m_ui.pickerEntityMode = false;
        ctx()->Overlay.SetIncludeSleepingEntities(false);
        ctx()->Overlay.SetWantsOverlayInput(false);
    }

    void DrawPicker(const PluginSDK::Snapshot& snap) {
        ctx()->Overlay.SetIncludeSleepingEntities(true);
        ctx()->Overlay.SetWantsOverlayInput(true);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            EndPickerMode();
            return;
        }

        ImVec2 screenSize(static_cast<float>(snap.ScreenWidth),
                          static_cast<float>(snap.ScreenHeight));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(screenSize);
        ImGui::Begin("##radar_picker", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                          | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground
                          | ImGuiWindowFlags_NoScrollbar);
        ImGui::InvisibleButton("##hit", screenSize);
        const bool clicked = ImGui::IsItemClicked();
        const ImVec2 mouse = ImGui::GetMousePos();
        ImGui::End();

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        if (m_ui.pickerPoiMode) {
            ctx()->Terrain.EnumerateTgtLocations([&](const PluginSDK::TgtLocation& loc) {
                const auto scr = RadarRender::ProjectGridToScreen(
                    const_cast<PluginSDK::Context*>(ctx()), snap, loc.X, loc.Y, 0.f);
                if (!scr.valid) return true;
                const float dx = mouse.x - scr.sx;
                const float dy = mouse.y - scr.sy;
                const bool hover = (dx * dx + dy * dy) < 144.f;
                dl->AddCircleFilled(ImVec2(scr.sx, scr.sy), 8.f,
                                    hover ? IM_COL32(0, 255, 255, 255)
                                          : IM_COL32(255, 255, 0, 200));
                if (clicked && hover) {
                    m_ui.editTarget = {};
                    m_ui.editTarget.path = loc.Path;
                    m_ui.editTarget.name = loc.Path;
                    m_ui.editTarget.category = "User";
                    m_ui.editTarget.enabled = true;
                    m_ui.editAreaKey = m_overlay.targets.ResolveAreaKey(snap.CurrentAreaHash,
                                                                        snap.CurrentAreaName);
                    m_ui.editIsNew = true;
                    m_ui.editModalOpen = true;
                    EndPickerMode();
                }
                return true;
            });
        }

        if (m_ui.pickerEntityMode) {
            for (const auto& e : snap.Entities) {
                if (!e.IsValid) continue;
                const auto scr = RadarRender::ProjectEntityToScreen(
                    const_cast<PluginSDK::Context*>(ctx()), snap, e);
                if (!scr.valid) continue;
                const float dx = mouse.x - scr.sx;
                const float dy = mouse.y - scr.sy;
                const bool hover = (dx * dx + dy * dy) < 144.f;
                dl->AddCircleFilled(ImVec2(scr.sx, scr.sy), 6.f,
                                    hover ? IM_COL32(255, 128, 0, 255)
                                          : IM_COL32(200, 200, 200, 180));
                if (clicked && hover) {
                    m_ui.editTarget = {};
                    m_ui.editTarget.path = std::string(e.Path.begin(), e.Path.end());
                    m_ui.editTarget.name = m_ui.editTarget.path;
                    m_ui.editTarget.category = "User";
                    m_ui.editTarget.enabled = true;
                    m_ui.editAreaKey = m_overlay.targets.ResolveAreaKey(snap.CurrentAreaHash,
                                                                        snap.CurrentAreaName);
                    m_ui.editIsNew = true;
                    m_ui.editModalOpen = true;
                    EndPickerMode();
                }
            }
        }

        dl->AddText(ImVec2(12, 12), IM_COL32(255, 255, 255, 255),
                    m_ui.pickerPoiMode ? "Click a POI on the map (Esc to cancel)"
                                       : "Click an entity on the map (Esc to cancel)");
    }
};

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() { return new RadarPlugin(); }

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* p) { delete p; }
