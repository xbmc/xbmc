/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Application.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "guilib/D3DResource.h"
#include "guilib/GraphicContext.h"
#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererDX.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

#ifndef ASSERT
#include <crtdbg.h>
#define ASSERT(f) _ASSERTE((f))
#endif

#ifdef HAS_DX

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
    CLog::Log(LOGERROR, "%s - failed to allocate texture.", __FUNCTION__);
    return false;
  }

  D3D11_TEXTURE2D_DESC desc = {};
  if (!texture->GetDesc(&desc))
  {
    CLog::Log(LOGERROR, "%s - failed to get texture description.", __FUNCTION__);
    texture->Release();
    return false;
  }
  ASSERT(format == desc.Format || (format == DXGI_FORMAT_R8_UNORM && desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM));

  *u = float(width) / desc.Width;
  *v = float(height) / desc.Height;

  return true;
}

COverlayQuadsDX::COverlayQuadsDX(ASS_Image* images, int width, int height)
{
  m_width  = 1.0;
  m_height = 1.0;
  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_RELATIVE;
  m_x      = 0.0f;
  m_y      = 0.0f;
  m_count  = 0;

  SQuads quads;
  if(!convert_quad(images, quads))
    return;
  
  float u, v;
  if(!LoadTexture(quads.size_x
                , quads.size_y
                , quads.size_x
                , DXGI_FORMAT_R8_UNORM
                , quads.data
                , &u, &v
                , &m_texture))
  {
    return;
  }

  Vertex* vt = new Vertex[6 * quads.count], *vt_orig = vt;
  SQuad*  vs = quads.quad;

  float scale_u = u / quads.size_x;
  float scale_v = v / quads.size_y;

  float scale_x = 1.0f / width;
  float scale_y = 1.0f / height;

  for (int i = 0; i < quads.count; i++)
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
  m_count = quads.count;

  if (!m_vertex.Create(D3D11_BIND_VERTEX_BUFFER, 6 * quads.count, sizeof(Vertex), DXGI_FORMAT_UNKNOWN, D3D11_USAGE_IMMUTABLE, vt))
  {
    CLog::Log(LOGERROR, "%s - failed to create vertex buffer", __FUNCTION__);
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

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  XMMATRIX world = pGUIShader->GetWorld();
  XMMATRIX view = pGUIShader->GetView();
  XMMATRIX proj = pGUIShader->GetProjection();

  if (g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
   || g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    CRect rect;
    g_Windowing.GetViewPort(rect);
    g_Windowing.SetCameraPosition(CPoint(rect.Width()*0.5f, rect.Height()*0.5f), rect.Width(), rect.Height());
  }

  XMMATRIX trans = XMMatrixTranslation(state.x, state.y, 0.0f);
  XMMATRIX scale = XMMatrixScaling(state.width, state.height, 1.0f);

  pGUIShader->SetWorld(XMMatrixMultiply(XMMatrixMultiply(world, scale), trans));

  const unsigned stride = sizeof(Vertex);
  const unsigned offset = 0;

  ID3D11Buffer* vertexBuffer = m_vertex.Get();
  // Set the vertex buffer to active in the input assembler so it can be rendered.
  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  g_Windowing.SetAlphaBlendEnable(true);
  pGUIShader->Begin(SHADER_METHOD_RENDER_FONT);

  ID3D11ShaderResourceView* views[] = { m_texture.GetShaderResource() };
  pGUIShader->SetShaderViews(1, views);
  pGUIShader->Draw(m_count * 6, 0);

  // restoring transformation
  pGUIShader->SetWorld(world);
  pGUIShader->SetView(view);
  pGUIShader->SetProjection(proj);
  pGUIShader->RestoreBuffers();
}

COverlayImageDX::~COverlayImageDX()
{
}

COverlayImageDX::COverlayImageDX(CDVDOverlayImage* o)
{
  uint32_t* rgba;
  int stride;
  if(o->palette)
  {
    m_pma  = !!USE_PREMULTIPLIED_ALPHA;
    rgba   = convert_rgba(o, m_pma);
    stride = o->width * 4;
  }
  else
  {
    m_pma  = false;
    rgba   = (uint32_t*)o->data;
    stride = o->linesize;
  }

  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayImageDX::COverlayImageDX - failed to convert overlay to rgb");
    return;
  }

  Load(rgba, o->width, o->height, stride);
  if((BYTE*)rgba != o->data)
    free(rgba);

  if(o->source_width && o->source_height)
  {
    float center_x = (float)(0.5f * o->width  + o->x) / o->source_width;
    float center_y = (float)(0.5f * o->height + o->y) / o->source_height;

    m_width  = (float)o->width  / o->source_width;
    m_height = (float)o->height / o->source_height;
    m_pos    = POSITION_RELATIVE;

#if 0
    if(center_x > 0.4 && center_x < 0.6
    && center_y > 0.8 && center_y < 1.0)
    {
     /* render bottom aligned to subtitle line */
      m_align  = ALIGN_SUBTITLE;
      m_x      = 0.0f;
      m_y      = - 0.5 * m_height;
    }
    else
#endif
    {
      /* render aligned to screen to avoid cropping problems */
      m_align  = ALIGN_SCREEN;
      m_x      = center_x;
      m_y      = center_y;
    }
  }
  else
  {
    m_align  = ALIGN_VIDEO;
    m_pos    = POSITION_ABSOLUTE;
    m_x      = (float)o->x;
    m_y      = (float)o->y;
    m_width  = (float)o->width;
    m_height = (float)o->height;
  }
}

COverlayImageDX::COverlayImageDX(CDVDOverlaySpu* o)
{
  int min_x, max_x, min_y, max_y;
  uint32_t* rgba = convert_rgba(o, USE_PREMULTIPLIED_ALPHA
                              , min_x, max_x, min_y, max_y);
  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayImageDX::COverlayImageDX - failed to convert overlay to rgb");
    return;
  }
  Load(rgba + min_x + min_y * o->width, max_x - min_x, max_y - min_y, o->width * 4);
  free(rgba);

  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)(min_x + o->x);
  m_y      = (float)(min_y + o->y);
  m_width  = (float)(max_x - min_x);
  m_height = (float)(max_y - min_y);
}

void COverlayImageDX::Load(uint32_t* rgba, int width, int height, int stride)
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
    CLog::Log(LOGERROR, "%s - failed to create vertex buffer", __FUNCTION__);
    m_texture.Release();
  }
}

void COverlayImageDX::Render(SRenderState &state)
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  XMMATRIX world = pGUIShader->GetWorld();
  XMMATRIX view = pGUIShader->GetView();
  XMMATRIX proj = pGUIShader->GetProjection();

  if (g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
   || g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    CRect rect;
    g_Windowing.GetViewPort(rect);
    g_Windowing.SetCameraPosition(CPoint(rect.Width()*0.5f, rect.Height()*0.5f), rect.Width(), rect.Height());
  }

  XMMATRIX trans = m_pos == POSITION_RELATIVE
                 ? XMMatrixTranslation(state.x - state.width  * 0.5f, state.y - state.height * 0.5f, 0.0f)
                 : XMMatrixTranslation(state.x, state.y, 0.0f),
           scale = XMMatrixScaling(state.width, state.height, 1.0f);

  pGUIShader->SetWorld(XMMatrixMultiply(XMMatrixMultiply(world, scale), trans));

  const unsigned stride = m_vertex.GetStride();
  const unsigned offset = 0;

  ID3D11Buffer* vertexBuffer = m_vertex.Get();
  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_NOBLEND);
  g_Windowing.SetAlphaBlendEnable(true);

  ID3D11ShaderResourceView* views[] = { m_texture.GetShaderResource() };
  pGUIShader->SetShaderViews(1, views);
  pGUIShader->Draw(4, 0);

  // restoring transformation
  pGUIShader->SetWorld(world);
  pGUIShader->SetView(view);
  pGUIShader->SetProjection(proj);
  pGUIShader->RestoreBuffers();
}

#endif
