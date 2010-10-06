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

#ifdef HAS_DX

#include "GUIFont.h"
#include "GUIFontTTFDX.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "gui3d.h"
#include "WindowingFactory.h"
#include "utils/log.h"

// stuff for freetype
#include "ft2build.h"

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

struct CUSTOMVERTEX 
{
  FLOAT x, y, z;
  DWORD color;
  FLOAT tu, tv;   // Texture coordinates
};


CGUIFontTTFDX::CGUIFontTTFDX(const CStdString& strFileName)
: CGUIFontTTFBase(strFileName)
{
  m_speedupTexture = NULL;
}

CGUIFontTTFDX::~CGUIFontTTFDX(void)
{
  SAFE_DELETE(m_speedupTexture);
}

void CGUIFontTTFDX::RenderInternal(SVertex* v)
{
  CUSTOMVERTEX verts[4] =  {
  { v[0].x-0.5f, v[0].y-0.5f, v[0].z, m_color, v[0].u, v[0].v},
  { v[1].x-0.5f, v[1].y-0.5f, v[1].z, m_color, v[1].u, v[1].v},
  { v[2].x-0.5f, v[2].y-0.5f, v[2].z, m_color, v[2].u, v[2].v},
  { v[3].x-0.5f, v[3].y-0.5f, v[3].z, m_color, v[3].u, v[3].v}
  };

  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
}

void CGUIFontTTFDX::Begin()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (m_nestedBeginCount == 0)
  {
    // just have to blit from our texture.
    pD3DDevice->SetTexture( 0, m_texture->GetTextureObject() );

    pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 ); // only use diffuse
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    // no other texture stages needed
    pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

    pD3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
  }
  // Keep track of the nested begin/end calls.
  m_vertex_count = 0;
  m_nestedBeginCount++;
}

void CGUIFontTTFDX::End()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  pD3DDevice->SetTexture(0, NULL);
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
}

CBaseTexture* CGUIFontTTFDX::ReallocTexture(unsigned int& newHeight)
{
  CBaseTexture* pNewTexture = new CDXTexture(m_textureWidth, newHeight, XB_FMT_A8);
  pNewTexture->CreateTextureObject();
  LPDIRECT3DTEXTURE9 newTexture = pNewTexture->GetTextureObject();

  if (newTexture == NULL)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to create the new texture h=%d w=%d", m_textureWidth, newHeight);
    SAFE_DELETE(pNewTexture);
    return NULL;
  }

  // Use a speedup texture in system memory when main texture in default pool+dynamic
  // Otherwise the texture would have to be copied from vid mem to sys mem, which is too slow for subs while playing video.
  CD3DTexture* newSpeedupTexture = NULL;
  if (g_Windowing.DefaultD3DPool() == D3DPOOL_DEFAULT && g_Windowing.DefaultD3DUsage() == D3DUSAGE_DYNAMIC)
  {
    newSpeedupTexture = new CD3DTexture();

    if (!newSpeedupTexture->Create(m_textureWidth, newHeight, 1, 0, D3DFMT_A8, D3DPOOL_SYSTEMMEM))
    {
      SAFE_DELETE(newSpeedupTexture);
      SAFE_DELETE(pNewTexture);
      return NULL;
    }
  }

  LPDIRECT3DSURFACE9 pSource, pTarget;
  HRESULT hr;
  // There might be data to copy from the previous texture
  if ((newSpeedupTexture && m_speedupTexture) || (newTexture && m_texture))
  {
    if (m_speedupTexture)
    {
      m_speedupTexture->GetSurfaceLevel(0, &pSource);
      newSpeedupTexture->GetSurfaceLevel(0, &pTarget);
    }
    else
    {
      m_texture->GetTextureObject()->GetSurfaceLevel(0, &pSource);
      newTexture->GetSurfaceLevel(0, &pTarget);
    }

    D3DLOCKED_RECT srclr, dstlr;
    if(FAILED(pSource->LockRect( &srclr, NULL, 0 ))
    || FAILED(pTarget->LockRect( &dstlr, NULL, 0 )))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - failed to lock surfaces");
      SAFE_DELETE(newSpeedupTexture);
      SAFE_DELETE(pNewTexture);
      pSource->Release();
      pTarget->Release();
      return NULL;
    }

    unsigned char *dst = (unsigned char *)dstlr.pBits;
    unsigned char *src = (unsigned char *)srclr.pBits;
    unsigned int dstPitch = dstlr.Pitch;
    unsigned int srcPitch = srclr.Pitch;
    unsigned int minPitch = std::min(srcPitch, dstPitch);

    if (srcPitch == dstPitch)
    {
      memcpy(dst, src, srcPitch * m_textureHeight);
    }
    else
    {
      for (unsigned int y = 0; y < m_textureHeight; y++)
      {
        memcpy(dst, src, minPitch);
        src += srcPitch;
        dst += dstPitch;
      }
    }
    pSource->UnlockRect();
    pTarget->UnlockRect();

    pSource->Release();
    pTarget->Release();
  }

  // Upload from speedup texture to main texture
  if (newSpeedupTexture && m_speedupTexture)
  {
    LPDIRECT3DSURFACE9 pSource, pTarget;
    newSpeedupTexture->GetSurfaceLevel(0, &pSource);
    newTexture->GetSurfaceLevel(0, &pTarget);
    const RECT rect = { 0, 0, m_textureWidth, m_textureHeight };
    const POINT point = { 0, 0 };

    hr = g_Windowing.Get3DDevice()->UpdateSurface(pSource, &rect, pTarget, &point);
    SAFE_RELEASE(pSource);
    SAFE_RELEASE(pTarget);

    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to upload from sysmem to vidmem (0x%08X)", hr);
      SAFE_DELETE(newSpeedupTexture);
      SAFE_DELETE(pNewTexture);
      return NULL;
    }
  }

  SAFE_DELETE(m_texture);
  SAFE_DELETE(m_speedupTexture);
  m_textureHeight = newHeight;
  m_speedupTexture = newSpeedupTexture;

  return pNewTexture;
}

bool CGUIFontTTFDX::CopyCharToTexture(FT_BitmapGlyph bitGlyph, Character* ch)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  LPDIRECT3DSURFACE9 target;
  if (m_speedupTexture)
    m_speedupTexture->GetSurfaceLevel(0, &target);
  else
    m_texture->GetTextureObject()->GetSurfaceLevel(0, &target);

  RECT sourcerect = { 0, 0, bitmap.width, bitmap.rows };
  RECT targetrect;
  targetrect.top = m_posY + ch->offsetY;
  targetrect.left = m_posX + bitGlyph->left;
  targetrect.bottom = targetrect.top + bitmap.rows;
  targetrect.right = targetrect.left + bitmap.width;
  
  HRESULT hr = D3DXLoadSurfaceFromMemory( target, NULL, &targetrect,
                                          bitmap.buffer, D3DFMT_LIN_A8, bitmap.pitch, NULL, &sourcerect,
                                          D3DX_FILTER_NONE, 0x00000000);

  SAFE_RELEASE(target);

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to copy the new character (0x%08X)", hr);
    return false;
  }

  if (m_speedupTexture)
  {
    // Upload to GPU - the automatic dirty region tracking takes care of the rect.
    HRESULT hr = g_Windowing.Get3DDevice()->UpdateTexture(m_speedupTexture->Get(), m_texture->GetTextureObject());
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to upload from sysmem to vidmem (0x%08X)", hr);
      return false;
    }
  }
  return TRUE;
}


void CGUIFontTTFDX::DeleteHardwareTexture()
{
  
}


#endif
