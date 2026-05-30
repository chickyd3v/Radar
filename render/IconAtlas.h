#pragma once

#include "data/IconTables.h"

#include <d3d11.h>
#include <imgui.h>

#include <filesystem>

struct ImDrawList;

namespace RadarRender {

class IconAtlas {
public:
    bool Load(void* d3dDevice, const std::filesystem::path& pngPath, int gridCols, int gridRows);
    void Release();
    bool Valid() const { return m_srv != nullptr; }

    void DrawIcon(ImDrawList* dl, int cx, int cy, float size, float screenX, float screenY,
                  ImU32 tint = IM_COL32(255, 255, 255, 255)) const;

    int GridCols() const { return m_cols; }
    int GridRows() const { return m_rows; }

private:
    ID3D11ShaderResourceView* m_srv = nullptr;
    int                       m_texW = 0;
    int                       m_texH = 0;
    int                       m_cols = 14;
    int                       m_rows = 71;
    float                     m_cellW = 32.f;
    float                     m_cellH = 32.f;
};

} // namespace RadarRender
