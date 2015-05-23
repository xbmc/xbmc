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

#ifdef HAS_DX

#include "GUIFontTTFDX.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "gui3d.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

// stuff for freetype
#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_GLYPH_H

using namespace std;

CGUIFontTTFDX::CGUIFontTTFDX(const std::string& strFileName)
: CGUIFontTTFBase(strFileName)
{
  m_speedupTexture = NULL;
  m_index      = NULL;
  m_index_size = 0;
}

CGUIFontTTFDX::~CGUIFontTTFDX(void)
{
  SAFE_DELETE(m_speedupTexture);
  free(m_index);
}

bool CGUIFontTTFDX::FirstBegin()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (pD3DDevice == NULL)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to get Direct3D device");
    return false;
  }

  int unit = 0;
  // just have to blit from our texture.
  m_texture->BindToUnit(unit);
  pD3DDevice->SetTextureStageState( unit, D3DTSS_COLOROP, D3DTOP_SELECTARG1 ); // only use diffuse
  pD3DDevice->SetTextureStageState( unit, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
  pD3DDevice->SetTextureStageState( unit, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  pD3DDevice->SetTextureStageState( unit, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  pD3DDevice->SetTextureStageState( unit, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  unit++;

  if(g_Windowing.UseLimitedColor())
  {
    pD3DDevice->SetTextureStageState( unit, D3DTSS_COLOROP  , D3DTOP_ADD );
    pD3DDevice->SetTextureStageState( unit, D3DTSS_COLORARG1, D3DTA_CURRENT) ;
    pD3DDevice->SetRenderState( D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(16,16,16,0) );
    pD3DDevice->SetTextureStageState( unit, D3DTSS_COLORARG2, D3DTA_TFACTOR );
    unit++;
  }

  // no other texture stages needed
  pD3DDevice->SetTextureStageState( unit, D3DTSS_COLOROP, D3DTOP_DISABLE);
  pD3DDevice->SetTextureStageState( unit, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

  pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

  pD3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
  return true;
}

void CGUIFontTTFDX::LastEnd()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (m_vertex.size() == 0)
    return;

  /* If the number of elements in m_vertex reduces, we can simply re-use the
   * first elements in m_index without any need to reallocate or reinitialise
   * it. To ensure we don't reallocate m_index any more frequently than
   * m_vertex, keep their respective high watermarks (m_index_size and
   * m_vertex.capacity()) in line.
   */
  unsigned index_size = m_vertex.capacity() * 6 / 4;
  if(m_index_size < index_size)
  {
    uint16_t* id  = (uint16_t*)calloc(index_size, sizeof(uint16_t));
    if(id == NULL)
      return;

    for(int i = 0, b = 0; i < m_vertex.capacity(); i += 4, b += 6)
    {
      id[b+0] = i + 0;
      id[b+1] = i + 1;
      id[b+2] = i + 2;
      id[b+3] = i + 2;
      id[b+4] = i + 3;
      id[b+5] = i + 0;
    }
    free(m_index);
    m_index      = id;
    m_index_size = index_size;
  }

  D3DXMATRIX orig;
  pD3DDevice->GetTransform(D3DTS_WORLD, &orig);

  D3DXMATRIX world = orig;
  D3DXMATRIX trans;

  D3DXMatrixTranslation(&trans, - 0.5f
                              , - 0.5f
                              ,   0.0f);
  D3DXMatrixMultiply(&world, &world, &trans);

  pD3DDevice->SetTransform(D3DTS_WORLD, &world);

  pD3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST
                                    , 0
                                    , m_vertex.size()
                                    , m_vertex.size() / 2
                                    , m_index
                                    , D3DFMT_INDEX16
                                    , &m_vertex[0]
                                    , sizeof(SVertex));
  pD3DDevice->SetTransform(D3DTS_WORLD, &orig);

  pD3DDevice->SetTexture(0, NULL);
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
}

CBaseTexture* CGUIFontTTFDX::ReallocTexture(unsigned int& newHeight)
{
  assert(newHeight != 0);
  assert(m_textureWidth != 0);
  if(m_textureHeight == 0)
  {
    delete m_texture;
    m_texture = NULL;
    delete m_speedupTexture;
    m_speedupTexture = NULL;
  }
  m_staticCache.Flush();
  m_dynamicCache.Flush();

  CDXTexture* pNewTexture = new CDXTexture(m_textureWidth, newHeight, XB_FMT_A8);
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
  // There might be data to copy from the previous texture
  if ((newSpeedupTexture && m_speedupTexture) || (newTexture && m_texture))
  {
    if (m_speedupTexture && newSpeedupTexture)
    {
      m_speedupTexture->GetSurfaceLevel(0, &pSource);
      newSpeedupTexture->GetSurfaceLevel(0, &pTarget);
    }
    else
    {
      ((CDXTexture *)m_texture)->GetTextureObject()->GetSurfaceLevel(0, &pSource);
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

    HRESULT hr = g_Windowing.Get3DDevice()->UpdateSurface(pSource, &rect, pTarget, &point);
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
  m_textureScaleY = 1.0f / m_textureHeight;
  m_speedupTexture = newSpeedupTexture;

  return pNewTexture;
}

bool CGUIFontTTFDX::CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  LPDIRECT3DTEXTURE9 texture = ((CDXTexture *)m_texture)->GetTextureObject();
  LPDIRECT3DSURFACE9 target;
  if (m_speedupTexture)
    m_speedupTexture->GetSurfaceLevel(0, &target);
  else
    texture->GetSurfaceLevel(0, &target);

  RECT sourcerect = { 0, 0, bitmap.width, bitmap.rows };
  RECT targetrect = { x1, y1, x2, y2 };

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
    HRESULT hr = g_Windowing.Get3DDevice()->UpdateTexture(m_speedupTexture->Get(), texture);
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
