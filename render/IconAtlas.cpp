#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#include "IconAtlas.h"

#include <imgui.h>

namespace RadarRender {

void IconAtlas::Release() {
    if (m_srv) {
        m_srv->Release();
        m_srv = nullptr;
    }
}

bool IconAtlas::Load(void* d3dDevice, const std::filesystem::path& pngPath,
                     int gridCols, int gridRows) {
    Release();
    if (!d3dDevice) return false;

    const std::string pathUtf8 = pngPath.string();
    int w = 0, h = 0, comp = 0;
    unsigned char* pixels = stbi_load(pathUtf8.c_str(), &w, &h, &comp, 4);
    if (!pixels || w <= 0 || h <= 0) return false;

    m_texW = w;
    m_texH = h;
    m_cols = gridCols > 1 ? gridCols : 1;
    m_rows = gridRows > 1 ? gridRows : 1;
    m_cellW = static_cast<float>(w) / static_cast<float>(m_cols);
    m_cellH = static_cast<float>(h) / static_cast<float>(m_rows);

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(w);
    desc.Height = static_cast<UINT>(h);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub{};
    sub.pSysMem = pixels;
    sub.SysMemPitch = static_cast<UINT>(w * 4);

    auto* dev = static_cast<ID3D11Device*>(d3dDevice);
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = dev->CreateTexture2D(&desc, &sub, &tex);
    stbi_image_free(pixels);
    if (FAILED(hr) || !tex) return false;

    hr = dev->CreateShaderResourceView(tex, nullptr, &m_srv);
    tex->Release();
    return SUCCEEDED(hr) && m_srv;
}

void IconAtlas::DrawIcon(ImDrawList* dl, int cx, int cy, float size, float screenX,
                         float screenY, ImU32 tint) const {
    if (!dl || !m_srv || size <= 0.f) return;
    const float u0 = (cx * m_cellW) / static_cast<float>(m_texW);
    const float v0 = (cy * m_cellH) / static_cast<float>(m_texH);
    const float u1 = ((cx + 1) * m_cellW) / static_cast<float>(m_texW);
    const float v1 = ((cy + 1) * m_cellH) / static_cast<float>(m_texH);
    const ImVec2 p0(screenX - size * 0.5f, screenY - size * 0.5f);
    const ImVec2 p1(screenX + size * 0.5f, screenY + size * 0.5f);
    dl->AddImage(reinterpret_cast<ImTextureID>(m_srv), p0, p1, ImVec2(u0, v0), ImVec2(u1, v1),
                 tint);
}

} // namespace RadarRender
