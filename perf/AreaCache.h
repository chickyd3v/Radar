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
    int                             lastTgtMatchCount = -1;
    int                             lastEntityMatchCount = -1;
    uint64_t                        lastTgtPollTime = 0;

    void Clear() {
        areaCounter = 0;
        walkablePtr = nullptr;
        walkable.Clear();
        entities.Clear();
        pois.Clear();
        entitySnapshotTime = 0;
        poiDirty = true;
        lastTgtMatchCount = -1;
        lastEntityMatchCount = -1;
        lastTgtPollTime = 0;
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
        lastTgtMatchCount = -1;
        lastEntityMatchCount = -1;
    }

    void RebuildEntitiesOnly(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                             const RadarData::RadarConfig& cfg,
                             const RadarData::TargetDatabase& db,
                            const RadarData::IconTables& icons) {
        entities.Rebuild(ctx, snap, cfg, db, icons);
        entitySnapshotTime = snap.LastUpdateTime;
    }

    void InvalidatePoi() { poiDirty = true; }

    // Rebuild when TGT tiles or matching entities appear (e.g. another Obelisk).
    void PollPoiDiscovery(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                          const RadarData::RadarConfig& cfg, const RadarData::TargetDatabase& db) {
        if (!cfg.ShowImportantPOI) return;
        if (!snap.LargeMap.IsVisible && !snap.MiniMap.IsVisible) return;
        if (snap.LastUpdateTime - lastTgtPollTime < 1500) return;
        lastTgtPollTime = snap.LastUpdateTime;

        const auto targets =
            db.GetTargetsForArea(snap.CurrentAreaHash, snap.CurrentAreaName);
        const int tgtCount = RadarRender::PoiDrawCache::CountMatchingTgtLocations(ctx, targets);
        const int entCount = RadarRender::PoiDrawCache::CountMatchingEntities(snap, targets);

        if (lastTgtMatchCount < 0 || lastEntityMatchCount < 0) {
            lastTgtMatchCount = tgtCount;
            lastEntityMatchCount = entCount;
            return;
        }
        if (tgtCount != lastTgtMatchCount || entCount != lastEntityMatchCount) {
            lastTgtMatchCount = tgtCount;
            lastEntityMatchCount = entCount;
            InvalidatePoi();
        }
    }

    void RebuildPoiIfNeeded(PluginSDK::Context* ctx, const PluginSDK::Snapshot& snap,
                            const RadarData::RadarConfig& cfg,
                            const RadarData::TargetDatabase& db,
                            const RadarData::IconTables& icons) {
        if (!poiDirty) return;
        pois.Rebuild(ctx, snap, cfg, db, icons);
        poiDirty = false;
        const auto targets =
            db.GetTargetsForArea(snap.CurrentAreaHash, snap.CurrentAreaName);
        lastTgtMatchCount = pois.CountMatchingTgtLocations(ctx, targets);
        lastEntityMatchCount = pois.CountMatchingEntities(snap, targets);
    }
};

} // namespace RadarPerf
