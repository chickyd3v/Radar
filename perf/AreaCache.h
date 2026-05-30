#pragma once

#include "render/EntityDrawCache.h"
#include "render/PoiDrawCache.h"
#include "render/WalkableBake.h"
#include "data/RadarConfig.h"
#include "data/TargetDatabase.h"
#include "data/IconTables.h"
#include "sdk/PluginSDK.h"


namespace RadarPerf {

struct AreaCacheState {
    uint64_t                        areaCounter = 0;
    const uint8_t*                  walkablePtr = nullptr;
    RadarRender::WalkableBake       walkable;
    RadarRender::EntityDrawCache    entities;
    RadarRender::PoiDrawCache       pois;
    uint64_t                        entitySnapshotTime = 0;
    bool                            poiDirty = true;

    void Clear() {
        areaCounter = 0;
        walkablePtr = nullptr;
        walkable.Clear();
        entities.Clear();
        pois.Clear();
        entitySnapshotTime = 0;
        poiDirty = true;
    }

    bool NeedsFullRebuild(const PluginSDK::Snapshot& snap, const uint8_t* walkData) const {
        return snap.AreaChangeCounter != areaCounter || walkData != walkablePtr;
    }

    bool NeedsEntityRebuild(const PluginSDK::Snapshot& snap) const {
        return snap.LastUpdateTime != entitySnapshotTime;
    }

    void RebuildAll(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                    PluginSDK::WalkableGridHandle& gridHandle,
                    const RadarData::RadarConfig& cfg, const RadarData::TargetDatabase& db,
                            const RadarData::IconTables& icons) {
        areaCounter = snap.AreaChangeCounter;
        walkablePtr = gridHandle.Data();
        walkable.Rebuild(ctx, gridHandle, cfg);
        pois.Rebuild(ctx, snap, cfg, db, icons);
        entities.Rebuild(ctx, snap, cfg, db, icons);
        entitySnapshotTime = snap.LastUpdateTime;
        poiDirty = false;
    }

    void RebuildEntitiesOnly(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                             const RadarData::RadarConfig& cfg,
                             const RadarData::TargetDatabase& db,
                            const RadarData::IconTables& icons) {
        entities.Rebuild(ctx, snap, cfg, db, icons);
        entitySnapshotTime = snap.LastUpdateTime;
    }

    void InvalidatePoi() { poiDirty = true; }

    void RebuildPoiIfNeeded(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                            const RadarData::RadarConfig& cfg,
                            const RadarData::TargetDatabase& db,
                            const RadarData::IconTables& icons) {
        if (!poiDirty) return;
        pois.Rebuild(ctx, snap, cfg, db, icons);
        poiDirty = false;
    }
};

} // namespace RadarPerf
