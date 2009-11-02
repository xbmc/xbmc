/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererDX.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "Application.h"
#include "WindowingFactory.h"

#ifdef HAS_DX

using namespace OVERLAY;

#define USE_PREMULTIPLIED_ALPHA 1

static bool LoadTexture(LPDIRECT3DDEVICE9 device
                      , int width, int height, int stride
                      , D3DFORMAT format
                      , const void* pixels
                      , float* u, float* v
                      , LPDIRECT3DTEXTURE9* texture)
{
  HRESULT result;
  result = D3DXCreateTexture(device
                           , width
                           , height
                           , 1, 0
                           , format
                           , D3DPOOL_MANAGED
                           , texture);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "LoadTexture - failed to allocate texture (%u)", result);
    return false;
  }

  int bpp;
  if     (format == D3DFMT_A8)
    bpp = 1;
  else if(format == D3DFMT_A8R8G8B8)
    bpp = 4;
  else
    ASSERT(0);

  D3DSURFACE_DESC desc;
  result = (*texture)->GetLevelDesc(0, &desc);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "LoadTexture - failed to get level description(%u)", result);
    SAFE_RELEASE(*texture);
    return false;
  }
  ASSERT(format == desc.Format);

  *u = (float)width  / desc.Width;
  *v = (float)height / desc.Height;

  D3DLOCKED_RECT lr;
  result = (*texture)->LockRect(0, &lr, NULL, 0);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "LoadTexture - failed to lock texture (%u)", result);
    SAFE_RELEASE(*texture);
    return false;
  }

  uint8_t* src   = (uint8_t*)pixels;
  uint8_t* dst   = (uint8_t*)lr.pBits;

  for (int y = 0; y < height; y++)
  {
    memcpy(dst, src, bpp * width);
    src += stride;
    dst += lr.Pitch;
  }

  if((unsigned)width < desc.Width)
  {
    uint8_t* src   = (uint8_t*)pixels   + bpp *(width - 1);
    uint8_t* dst   = (uint8_t*)lr.pBits + bpp * width;
    for (int y = 0; y < height; y++)
    {
      memcpy(dst, src, bpp);
      src += stride;
      dst += lr.Pitch;
    }
  }

  if((unsigned)height < desc.Height)
  {
    uint8_t* src   = (uint8_t*)pixels   + stride   * (height - 1);
    uint8_t* dst   = (uint8_t*)lr.pBits + lr.Pitch * height;
    memcpy(dst, src, bpp * width);
  }

  (*texture)->UnlockRect(0);

  return true;
}

COverlayQuadsDX::COverlayQuadsDX(CDVDOverlaySSA* o, double pts)
{
  m_vertex = NULL;
  m_width  = (float)g_graphicsContext.GetWidth();
  m_height = (float)g_graphicsContext.GetHeight();
  m_device = g_Windowing.Get3DDevice();
  m_fvf    = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
  m_align  = ALIGN_SCREEN;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)0.0f;
  m_y      = (float)0.0f;


  SQuads quads;
  if(!convert_quad(o, pts, (int)m_width, (int)m_height, quads))
    return;

  float u, v;
  if(!LoadTexture(m_device
                , quads.size_x
                , quads.size_y
                , quads.size_x
                , D3DFMT_A8
                , quads.data
                , &u, &v
                , &m_texture))
  {
    return;
  }

  HRESULT result;
  result = m_device->CreateVertexBuffer(sizeof(VERTEX) * 6 * quads.count, 0, m_fvf, D3DPOOL_MANAGED, &m_vertex, NULL);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "COverlayQuadsDX::COverlayQuadsDX - failed to create vertex buffer (%u)", result);
    SAFE_RELEASE(m_texture);
    return;
  }

  VERTEX* vt = NULL;
  SQuad*  vs = quads.quad;

  result = m_vertex->Lock(0, 0, (void**)&vt, D3DLOCK_DISCARD);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "COverlayQuadsDX::COverlayQuadsDX - failed to lock texture (%u)", result);
    SAFE_RELEASE(m_texture);
    return;
  }

  float scale_u = u    / quads.size_x;
  float scale_v = v    / quads.size_y;

  float scale_x = 1.0f / m_width;
  float scale_y = 1.0f / m_height;

  for(int i = 0; i < quads.count; i++)
  {
    for(int s = 0; s < 6; s++)
    {
      vt[s].c = D3DCOLOR_ARGB(vs->a, vs->r, vs->g, vs->b);
      vt[s].z = 0.0f;
      vt[s].x = scale_x;
      vt[s].y = scale_y;
      vt[s].u = scale_u;
      vt[s].v = scale_v;
    }

    vt[0].x *= vs->x;
    vt[0].u *= vs->u;
    vt[0].y *= vs->y;
    vt[0].v *= vs->v;

    vt[1].x *= vs->x + vs->w;
    vt[1].u *= vs->u + vs->w;
    vt[1].y *= vs->y;
    vt[1].v *= vs->v;

    vt[2].x *= vs->x;
    vt[2].u *= vs->u;
    vt[2].y *= vs->y + vs->h;
    vt[2].v *= vs->v + vs->h;

    vt[3] = vt[1];

    vt[4].x *= vs->x + vs->w;
    vt[4].u *= vs->u + vs->w;
    vt[4].y *= vs->y + vs->h;
    vt[4].v *= vs->v + vs->h;

    vt[5] = vt[2];

    vs += 1;
    vt += 6;
  }

  m_vertex->Unlock();
  m_count  = quads.count;
}

COverlayQuadsDX::~COverlayQuadsDX()
{
  SAFE_RELEASE(m_vertex);
  SAFE_RELEASE(m_texture);
}

void COverlayQuadsDX::Render(SRenderState &state)
{
  D3DXMATRIX orig;
  m_device->GetTransform(D3DTS_WORLD, &orig);

  D3DXMATRIX world = orig;
  D3DXMATRIX trans, scale;

  D3DXMatrixTranslation(&trans, state.x - 0.5f
                              , state.y - 0.5f
                              , 0.0f);

  D3DXMatrixScaling    (&scale, state.width
                              , state.height
                              , 1.0f);

  D3DXMatrixMultiply(&world, &world, &scale);
  D3DXMatrixMultiply(&world, &world, &trans);

  m_device->SetTransform(D3DTS_WORLD, &world);

  m_device->SetTexture( 0, m_texture );
  m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP);
  m_device->SetSamplerState(0, D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_SELECTARG1 );
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
  m_device->SetTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_MODULATE );
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

  m_device->SetRenderState( D3DRS_LIGHTING , FALSE );
  m_device->SetRenderState( D3DRS_ZENABLE  , FALSE );
  m_device->SetRenderState( D3DRS_FOGENABLE, FALSE );

  m_device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  m_device->SetRenderState( D3DRS_ALPHAREF , 0 );
  m_device->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  m_device->SetRenderState( D3DRS_FILLMODE , D3DFILL_SOLID );
  m_device->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );

  m_device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  m_device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
  m_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

  m_device->SetFVF(m_fvf);
  m_device->SetStreamSource(0, m_vertex, 0, sizeof(VERTEX));
  m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_count*2);
}

COverlayImageDX::~COverlayImageDX()
{
  SAFE_RELEASE(m_vertex);
  SAFE_RELEASE(m_texture);
}

COverlayImageDX::COverlayImageDX(CDVDOverlayImage* o)
{
  uint32_t* rgba = convert_rgba(o, USE_PREMULTIPLIED_ALPHA);
  if(!rgba)
  {
    CLog::Log(LOGERROR, "COverlayImageDX::COverlayImageDX - failed to convert overlay to rgb");
    return;
  }
  Load(rgba, o->width, o->height, o->width * 4);

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

  m_align  = ALIGN_VIDEO;
  m_pos    = POSITION_ABSOLUTE;
  m_x      = (float)(min_x + o->x);
  m_y      = (float)(min_y + o->y);
  m_width  = (float)(max_x - min_x);
  m_height = (float)(max_y - min_y);
}

void COverlayImageDX::Load(uint32_t* rgba, int width, int height, int stride)
{
  m_device = g_Windowing.Get3DDevice();
  m_fvf    = D3DFVF_XYZ | D3DFVF_TEX1;

  float u, v;
  if(!LoadTexture(m_device
                , width
                , height
                , stride
                , D3DFMT_A8R8G8B8
                , rgba
                , &u, &v
                , &m_texture))
    return;

  HRESULT result;
  result = m_device->CreateVertexBuffer(sizeof(VERTEX) * 6, 0, m_fvf, D3DPOOL_MANAGED, &m_vertex, NULL);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "COverlayImageDX::Load - failed to create vertex buffer (%u)", result);
    return;
  }

  VERTEX*  vt = NULL;;
  result = m_vertex->Lock(0, 0, (void**)&vt, D3DLOCK_DISCARD);
  if(FAILED(result))
  {
    CLog::Log(LOGERROR, "COverlayImageDX::Load - failed to lock texture (%u)", result);
    SAFE_RELEASE(m_texture);
    SAFE_RELEASE(m_vertex);
    return;
  }

  vt[0].u = 0.0f;
  vt[0].v = 0.0f;
  vt[0].x = 0.0f;
  vt[0].y = 0.0f;
  vt[0].z = 0.0f;

  vt[1].u = u;
  vt[1].v = 0.0f;
  vt[1].x = 1.0f;
  vt[1].y = 0.0f;
  vt[1].z = 0.0f;

  vt[2].u = 0.0f;
  vt[2].v = v;
  vt[2].x = 0.0f;
  vt[2].y = 1.0f;
  vt[2].z = 0.0f;

  vt[3] = vt[1];

  vt[4].u = u;
  vt[4].v = v;
  vt[4].x = 1.0f;
  vt[4].y = 1.0f;
  vt[4].z = 0.0f;

  vt[5] = vt[2];

  m_vertex->Unlock();
}

void COverlayImageDX::Render(SRenderState &state)
{

  D3DXMATRIX orig;
  m_device->GetTransform(D3DTS_WORLD, &orig);

  D3DXMATRIX world = orig;
  D3DXMATRIX trans, scale;

  if(m_pos == POSITION_RELATIVE)
    D3DXMatrixTranslation(&trans, state.x - state.width  * 0.5f - 0.5f
                                , state.y - state.height * 0.5f - 0.5f
                                , 0.0f);
  else
    D3DXMatrixTranslation(&trans, state.x - 0.5f
                                , state.y - 0.5f
                                , 0.0f);

  D3DXMatrixScaling    (&scale, state.width
                              , state.height
                              , 1.0f);

  D3DXMatrixMultiply(&world, &world, &scale);
  D3DXMatrixMultiply(&world, &world, &trans);

  m_device->SetTransform(D3DTS_WORLD, &world);

  m_device->SetTexture( 0, m_texture );
  m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP);
  m_device->SetSamplerState(0, D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_SELECTARG1 );
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_device->SetTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_SELECTARG1 );
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

  m_device->SetRenderState( D3DRS_LIGHTING , FALSE );
  m_device->SetRenderState( D3DRS_ZENABLE  , FALSE );
  m_device->SetRenderState( D3DRS_FOGENABLE, FALSE );

  m_device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  m_device->SetRenderState( D3DRS_ALPHAREF , 0 );
  m_device->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  m_device->SetRenderState( D3DRS_FILLMODE , D3DFILL_SOLID );
  m_device->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );

  m_device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

#if USE_PREMULTIPLIED_ALPHA
  m_device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_ONE );
#else
  m_device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
#endif
  m_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

  m_device->SetFVF(m_fvf);
  m_device->SetStreamSource(0, m_vertex, 0, sizeof(VERTEX));
  m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

  m_device->SetTransform(D3DTS_WORLD, &orig);
}

#endif