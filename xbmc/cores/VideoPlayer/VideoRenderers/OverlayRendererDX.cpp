/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRendererDX.h"

#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "application/Application.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "guilib/D3DResource.h"
#include "guilib/GUIShaderDX.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#ifndef ASSERT
#include <crtdbg.h>
#define ASSERT(f) _ASSERTE((f))
#endif

#include <cmath>

using namespace OVERLAY;
using namespace DirectX;

#define USE_PREMULTIPLIED_ALPHA 1
#define ALPHA_CHANNEL_OFFSET 3

static bool LoadTexture(int width, int height, int stride
                      , DXGI_FORMAT format
                      , const void* pixels
                      , float* u, float* v
                      , CD3DTexture* texture)
{
  if (!texture->Create(width, height, 1, D3D11_USAGE_IMMUTABLE, format, pixels, stride))
  {
    CLog::Log(LOGERROR, "{} - failed to allocate texture.", __FUNCTION__);
    return false;
  }

  D3D11_TEXTURE2D_DESC desc = {};
  if (!texture->GetDesc(&desc))
  {
    CLog::Log(LOGERROR, "{} - failed to get texture description.", __FUNCTION__);
    texture->Release();
    return false;
  }
  ASSERT(format == desc.Format || (format == DXGI_FORMAT_R8_UNORM && desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM));

  *u = float(width) / desc.Width;
  *v = float(height) / desc.Height;

  return true;
}

std::shared_ptr<COverlay> COverlay::Create(ASS_Image* images, float width, float height)
{
  return std::make_shared<COverlayQuadsDX>(images, width, height);
}

COverlayQuadsDX::COverlayQuadsDX(ASS_Image* images, float width, float height)
{
  m_width  = 1.0;
  m_height = 1.0;
  m_align = ALIGN_SCREEN;
  m_pos    = POSITION_RELATIVE;
  m_x      = 0.0f;
  m_y      = 0.0f;
  m_count  = 0;

  SQuads quads;
  if (!convert_quad(images, quads, static_cast<int>(width)))
    return;

  float u, v;
  if (!LoadTexture(quads.size_x, quads.size_y, quads.size_x, DXGI_FORMAT_R8_UNORM,
                   quads.texture.data(), &u, &v, &m_texture))
  {
    return;
  }

  Vertex* vt = new Vertex[6 * quads.quad.size()];
  Vertex* vt_orig = vt;
  SQuad* vs = quads.quad.data();

  float scale_u = u / quads.size_x;
  float scale_v = v / quads.size_y;

  float scale_x = 1.0f / width;
  float scale_y = 1.0f / height;

  for (size_t i = 0; i < quads.quad.size(); i++)
  {
    for (int s = 0; s < 6; s++)
    {
      CD3DHelper::XMStoreColor(&vt[s].color, vs->a, vs->r, vs->g, vs->b);
      vt[s].pos = XMFLOAT3(scale_x, scale_y, 0.0f);
      vt[s].texCoord = XMFLOAT2(scale_u, scale_v);
      vt[s].texCoord2 = XMFLOAT2(0.0f, 0.0f);
    }

    vt[0].pos.x *= vs->x;
    vt[0].texCoord.x *= vs->u;
    vt[0].pos.y *= vs->y;
    vt[0].texCoord.y *= vs->v;

    vt[1].pos.x *= vs->x + vs->w;
    vt[1].texCoord.x *= vs->u + vs->w;
    vt[1].pos.y *= vs->y;
    vt[1].texCoord.y *= vs->v;

    vt[2].pos.x *= vs->x;
    vt[2].texCoord.x *= vs->u;
    vt[2].pos.y *= vs->y + vs->h;
    vt[2].texCoord.y *= vs->v + vs->h;

    vt[3] = vt[1];

    vt[4].pos.x *= vs->x + vs->w;
    vt[4].texCoord.x *= vs->u + vs->w;
    vt[4].pos.y *= vs->y + vs->h;
    vt[4].texCoord.y *= vs->v + vs->h;

    vt[5] = vt[2];

    vs += 1;
    vt += 6;
  }

  vt = vt_orig;
  m_count = static_cast<unsigned int>(quads.quad.size());

  if (!m_vertex.Create(D3D11_BIND_VERTEX_BUFFER, 6 * m_count, sizeof(Vertex), DXGI_FORMAT_UNKNOWN,
                       D3D11_USAGE_IMMUTABLE, vt))
  {
    CLog::Log(LOGERROR, "{} - failed to create vertex buffer", __FUNCTION__);
    m_texture.Release();
  }

  delete[] vt;
}

COverlayQuadsDX::~COverlayQuadsDX()
{
}

void COverlayQuadsDX::Render(SRenderState &state)
{
  if (m_count == 0)
    return;

  ID3D11Buffer* vertexBuffer = m_vertex.Get();
  if (vertexBuffer == nullptr)
    return;

  ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();

  XMMATRIX world, view, proj;
  pGUIShader->GetWVP(world, view, proj);

  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
   || CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    CRect rect;
    DX::Windowing()->GetViewPort(rect);
    DX::Windowing()->SetCameraPosition(CPoint(rect.Width() * 0.5f, rect.Height() * 0.5f),
                                  static_cast<int>(rect.Width()),
                                  static_cast<int>(rect.Height()));
  }

  XMMATRIX trans = XMMatrixTranslation(state.x, state.y, 0.0f);
  XMMATRIX scale = XMMatrixScaling(state.width, state.height, 1.0f);

  pGUIShader->SetWorld(XMMatrixMultiply(XMMatrixMultiply(world, scale), trans));

  const unsigned stride = sizeof(Vertex);
  const unsigned offset = 0;

  // Set the vertex buffer to active in the input assembler so it can be rendered.
  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  DX::Windowing()->SetAlphaBlendEnable(true);
  pGUIShader->Begin(SHADER_METHOD_RENDER_FONT);

  pGUIShader->SetShaderViews(1, m_texture.GetAddressOfSRV());
  pGUIShader->Draw(m_count * 6, 0);

  // restoring transformation
  pGUIShader->SetWVP(world, view, proj);
  pGUIShader->RestoreBuffers();
}

std::shared_ptr<COverlay> COverlay::Create(const CDVDOverlayImage& o, CRect& rSource)
{
  return std::make_shared<COverlayImageDX>(o, rSource);
}

COverlayImageDX::~COverlayImageDX()
{
}

COverlayImageDX::COverlayImageDX(const CDVDOverlayImage& o, CRect& rSource)
{
  if (o.palette.empty())
  {
    m_pma = false;
    const uint32_t* rgba = reinterpret_cast<const uint32_t*>(o.pixels.data());
    Load(rgba, o.width, o.height, o.linesize);
  }
  else
  {
    std::vector<uint32_t> rgba(o.width * o.height);
    m_pma = !!USE_PREMULTIPLIED_ALPHA;
    convert_rgba(o, m_pma, rgba);
    Load(rgba.data(), o.width, o.height, o.width * 4);
  }

  if (o.source_width > 0 && o.source_height > 0)
  {
    m_pos = POSITION_RELATIVE;
    m_x = (0.5f * o.width + o.x) / o.source_width;
    m_y = (0.5f * o.height + o.y) / o.source_height;

    const float subRatio{static_cast<float>(o.source_width) / o.source_height};
    const float vidRatio{rSource.Width() / rSource.Height()};

    // We always consider aligning 4/3 subtitles to the video,
    // for example SD DVB subtitles (4/3) must be stretched on fullhd video

    if (std::fabs(subRatio - vidRatio) < 0.001f || IsSquareResolution(subRatio))
    {
      m_align = ALIGN_VIDEO;
      m_width = static_cast<float>(o.width) / o.source_width;
      m_height = static_cast<float>(o.height) / o.source_height;
    }
    else
    {
      // We should have a re-encoded/cropped (removed black bars) video source.
      // Then we cannot align to video otherwise the subtitles will be deformed
      // better align to screen by keeping the aspect-ratio.
      m_align = ALIGN_SCREEN_AR;
      m_width = static_cast<float>(o.width);
      m_height = static_cast<float>(o.height);
      m_source_width = static_cast<float>(o.source_width);
      m_source_height = static_cast<float>(o.source_height);
    }
  }
  else
  {
    m_align = ALIGN_VIDEO;
    m_pos = POSITION_ABSOLUTE;
    m_x = static_cast<float>(o.x);
    m_y = static_cast<float>(o.y);
    m_width = static_cast<float>(o.width);
    m_height = static_cast<float>(o.height);
  }
}

std::shared_ptr<COverlay> COverlay::Create(const CDVDOverlaySpu& o)
{
  return std::make_shared<COverlayImageDX>(o);
}

COverlayImageDX::COverlayImageDX(const CDVDOverlaySpu& o)
{
  int min_x, max_x, min_y, max_y;
  std::vector<uint32_t> rgba(o.width * o.height);

  convert_rgba(o, USE_PREMULTIPLIED_ALPHA, min_x, max_x, min_y, max_y, rgba);
  Load(rgba.data() + min_x + min_y * o.width, max_x - min_x, max_y - min_y, o.width * 4);

  m_align = ALIGN_VIDEO;
  m_pos = POSITION_ABSOLUTE;
  m_x = static_cast<float>(min_x + o.x);
  m_y = static_cast<float>(min_y + o.y);
  m_width = static_cast<float>(max_x - min_x);
  m_height = static_cast<float>(max_y - min_y);
}

void COverlayImageDX::Load(const uint32_t* rgba, int width, int height, int stride)
{
  float u, v;
  if(!LoadTexture(width
                , height
                , stride
                , DXGI_FORMAT_B8G8R8A8_UNORM
                , rgba
                , &u, &v
                , &m_texture))
    return;

  Vertex vt[4];

  vt[0].texCoord = XMFLOAT2(u, 0.0f);
  vt[0].pos      = XMFLOAT3(1.0f, 0.0f, 0.0f);

  vt[1].texCoord = XMFLOAT2(u, v);
  vt[1].pos      = XMFLOAT3(1.0f, 1.0f, 0.0f);

  vt[2].texCoord = XMFLOAT2(0.0f, 0.0f);
  vt[2].pos      = XMFLOAT3(0.0f, 0.0f, 0.0f);

  vt[3].texCoord = XMFLOAT2(0.0f, v);
  vt[3].pos      = XMFLOAT3(0.0f, 1.0f, 0.0f);

  if (!m_vertex.Create(D3D11_BIND_VERTEX_BUFFER, 4, sizeof(Vertex), DXGI_FORMAT_UNKNOWN, D3D11_USAGE_IMMUTABLE, vt))
  {
    CLog::Log(LOGERROR, "{} - failed to create vertex buffer", __FUNCTION__);
    m_texture.Release();
  }
}

void COverlayImageDX::Render(SRenderState &state)
{
  ID3D11Buffer* vertexBuffer = m_vertex.Get();
  if (vertexBuffer == nullptr)
    return;

  ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();

  XMMATRIX world, view, proj;
  pGUIShader->GetWVP(world, view, proj);

  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
   || CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    CRect rect;
    DX::Windowing()->GetViewPort(rect);
    DX::Windowing()->SetCameraPosition(CPoint(rect.Width() * 0.5f, rect.Height() * 0.5f),
                                  static_cast<int>(rect.Width()),
                                  static_cast<int>(rect.Height()));
  }

  XMMATRIX trans = m_pos == POSITION_RELATIVE
                 ? XMMatrixTranslation(state.x - state.width  * 0.5f, state.y - state.height * 0.5f, 0.0f)
                 : XMMatrixTranslation(state.x, state.y, 0.0f),
           scale = XMMatrixScaling(state.width, state.height, 1.0f);

  pGUIShader->SetWorld(XMMatrixMultiply(XMMatrixMultiply(world, scale), trans));

  const unsigned stride = m_vertex.GetStride();
  const unsigned offset = 0;

  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_NOBLEND);
  DX::Windowing()->SetAlphaBlendEnable(true);

  pGUIShader->SetShaderViews(1, m_texture.GetAddressOfSRV());
  pGUIShader->Draw(4, 0);

  // restoring transformation
  pGUIShader->SetWVP(world, view, proj);
  pGUIShader->RestoreBuffers();
}
