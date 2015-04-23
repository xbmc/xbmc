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

#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "OverlayRendererDX.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlayImage.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "cores/dvdplayer/DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

#ifndef ASSERT
#include <crtdbg.h>
#define ASSERT(f) _ASSERTE((f))
#endif

#ifdef HAS_DX

using namespace OVERLAY;

#define USE_PREMULTIPLIED_ALPHA 1
#define ALPHA_CHANNEL_OFFSET 3

static bool LoadTexture(int width, int height, int stride
                      , D3DFORMAT format
                      , const void* pixels
                      , float* u, float* v
                      , CD3DTexture* texture)
{

  if (!texture->Create(width, height, 1, g_Windowing.DefaultD3DUsage(), format, g_Windowing.DefaultD3DPool()))
  {
    CLog::Log(LOGERROR, "LoadTexture - failed to allocate texture");
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
  if(!texture->GetLevelDesc(0, &desc))
  {
    CLog::Log(LOGERROR, "LoadTexture - failed to get level description");
    texture->Release();
    return false;
  }
  ASSERT(format == desc.Format || (format == D3DFMT_A8 && desc.Format == D3DFMT_A8R8G8B8));

  // Some old hardware doesn't have D3DFMT_A8 and returns D3DFMT_A8R8G8B8 textures instead
  int destbpp;
  if     (desc.Format == D3DFMT_A8)
    destbpp = 1;
  else if(desc.Format == D3DFMT_A8R8G8B8)
    destbpp = 4;
  else
    ASSERT(0);

  *u = (float)width  / desc.Width;
  *v = (float)height / desc.Height;

  D3DLOCKED_RECT lr;
  if (!texture->LockRect(0, &lr, NULL, D3DLOCK_DISCARD))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock texture");
    texture->Release();
    return false;
  }

  uint8_t* src   = (uint8_t*)pixels;
  uint8_t* dst   = (uint8_t*)lr.pBits;

  if (bpp == destbpp)
  {
    for (int y = 0; y < height; y++)
    {
      memcpy(dst, src, bpp * width);
      src += stride;
      dst += lr.Pitch;
    }
  }
  else if (bpp == 1 && destbpp == 4)
  {
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
        dst[x*destbpp + ALPHA_CHANNEL_OFFSET] = src[x];
      src += stride;
      dst += lr.Pitch;
    }
  }

  if((unsigned)width < desc.Width)
  {
    uint8_t* src   = (uint8_t*)pixels   + bpp *(width - 1);
    uint8_t* dst   = (uint8_t*)lr.pBits + bpp * width;

    if (bpp == destbpp)
    {
      for (int y = 0; y < height; y++)
      {
        memcpy(dst, src, bpp);
        src += stride;
        dst += lr.Pitch;
      }
    }
    else if (bpp == 1 && destbpp == 4)
    {
      for (int y = 0; y < height; y++)
      {
        dst[ALPHA_CHANNEL_OFFSET] = src[0];
        src += stride;
        dst += lr.Pitch;
      }
    }
  }

  if((unsigned)height < desc.Height)
  {
    uint8_t* src   = (uint8_t*)pixels   + stride   * (height - 1);
    uint8_t* dst   = (uint8_t*)lr.pBits + lr.Pitch * height;

    if (bpp == destbpp)
      memcpy(dst, src, bpp * width);
    else if (bpp == 1 && destbpp == 4)
    {
      for (int x = 0; x < width; x++)
        dst[x*destbpp + ALPHA_CHANNEL_OFFSET] = src[x];
    }
  }

  if (!texture->UnlockRect(0))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock texture");
    texture->Release();
    return false;
  }

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
  m_fvf    = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  SQuads quads;
  if(!convert_quad(images, quads))
    return;
  
  float u, v;
  if(!LoadTexture(quads.size_x
                , quads.size_y
                , quads.size_x
                , D3DFMT_A8
                , quads.data
                , &u, &v
                , &m_texture))
  {
    return;
  }

  if (!m_vertex.Create(sizeof(VERTEX) * 6 * quads.count, D3DUSAGE_WRITEONLY, m_fvf, g_Windowing.DefaultD3DPool()))
  {
    CLog::Log(LOGERROR, "%s - failed to create vertex buffer", __FUNCTION__);
    m_texture.Release();
    return;
  }

  VERTEX* vt = NULL;
  SQuad*  vs = quads.quad;

  if (!m_vertex.Lock(0, 0, (void**)&vt, 0))
  {
    CLog::Log(LOGERROR, "%s - failed to lock vertex buffer", __FUNCTION__);
    m_texture.Release();
    return;
  }

  float scale_u = u    / quads.size_x;
  float scale_v = v    / quads.size_y;

  float scale_x = 1.0f / width;
  float scale_y = 1.0f / height;

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

  m_vertex.Unlock();
  m_count  = quads.count;
}

COverlayQuadsDX::~COverlayQuadsDX()
{
}

void COverlayQuadsDX::Render(SRenderState &state)
{
  if (m_count == 0)
    return;

  D3DXMATRIX orig;
  LPDIRECT3DDEVICE9 device = g_Windowing.Get3DDevice();
  device->GetTransform(D3DTS_WORLD, &orig);

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

  device->SetTransform(D3DTS_WORLD, &world);

  device->SetTexture( 0, m_texture.Get() );
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP);
  device->SetSamplerState(0, D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP);

  device->SetTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_SELECTARG1 );
  device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
  device->SetTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_MODULATE );
  device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

  device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  device->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

  device->SetRenderState( D3DRS_LIGHTING , FALSE );
  device->SetRenderState( D3DRS_ZENABLE  , FALSE );
  device->SetRenderState( D3DRS_FOGENABLE, FALSE );

  device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  device->SetRenderState( D3DRS_ALPHAREF , 0 );
  device->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  device->SetRenderState( D3DRS_FILLMODE , D3DFILL_SOLID );
  device->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );

  device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
  device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  device->SetFVF(m_fvf);
  device->SetStreamSource(0, m_vertex.Get(), 0, sizeof(VERTEX));
  device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_count*2);

  device->SetTexture(0, NULL);
  device->SetTransform(D3DTS_WORLD, &orig);
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
  m_fvf    = D3DFVF_XYZ | D3DFVF_TEX1;

  float u, v;
  if(!LoadTexture(width
                , height
                , stride
                , D3DFMT_A8R8G8B8
                , rgba
                , &u, &v
                , &m_texture))
    return;

  if (!m_vertex.Create(sizeof(VERTEX) * 6, D3DUSAGE_WRITEONLY, m_fvf, g_Windowing.DefaultD3DPool()))
  {
    CLog::Log(LOGERROR, "%s - failed to create vertex buffer", __FUNCTION__);
    m_texture.Release();
    return;
  }

  VERTEX*  vt = NULL;
  if (!m_vertex.Lock(0, 0, (void**)&vt, 0))
  {
    CLog::Log(LOGERROR, "%s - failed to lock texture", __FUNCTION__);
    m_texture.Release();
    m_vertex.Release();
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

  m_vertex.Unlock();
}

void COverlayImageDX::Render(SRenderState &state)
{

  D3DXMATRIX orig;
  LPDIRECT3DDEVICE9 device = g_Windowing.Get3DDevice();
  device->GetTransform(D3DTS_WORLD, &orig);

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

  device->SetTransform(D3DTS_WORLD, &world);

  device->SetTexture( 0, m_texture.Get() );
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP);
  device->SetSamplerState(0, D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP);

  device->SetTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_SELECTARG1 );
  device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  device->SetTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_SELECTARG1 );
  device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

  device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  device->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

  device->SetRenderState( D3DRS_LIGHTING , FALSE );
  device->SetRenderState( D3DRS_ZENABLE  , FALSE );
  device->SetRenderState( D3DRS_FOGENABLE, FALSE );

  device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
  device->SetRenderState( D3DRS_ALPHAREF , 0 );
  device->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
  device->SetRenderState( D3DRS_FILLMODE , D3DFILL_SOLID );
  device->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );
  device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

#if USE_PREMULTIPLIED_ALPHA
  device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_ONE );
#else
  device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
#endif
  device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

  device->SetFVF(m_fvf);
  device->SetStreamSource(0, m_vertex.Get(), 0, sizeof(VERTEX));
  device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

  device->SetTexture( 0, NULL );
  device->SetTransform(D3DTS_WORLD, &orig);
}

#endif
