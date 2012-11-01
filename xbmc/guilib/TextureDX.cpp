/*
*      Copyright (C) 2005-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "TextureDX.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

#ifdef HAS_DX

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
CDXTexture::CDXTexture(unsigned int width, unsigned int height, unsigned int format)
: CBaseTexture(width, height, format)
{
}

CDXTexture::~CDXTexture()
{
  DestroyTextureObject();
}

void CDXTexture::CreateTextureObject()
{
  D3DFORMAT format = D3DFMT_UNKNOWN;

  switch (m_format)
  {
  case XB_FMT_DXT1:
    format = D3DFMT_DXT1;
    break;
  case XB_FMT_DXT3:
    format = D3DFMT_DXT3;
    break;
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    format = D3DFMT_DXT5;
    break;
  case XB_FMT_RGB8:
  case XB_FMT_A8R8G8B8:
    format = D3DFMT_A8R8G8B8;
    break;
  case XB_FMT_A8:
    format = D3DFMT_A8;
    break;
  default:
    return;
  }

  m_texture.Create(m_textureWidth, m_textureHeight, 1, g_Windowing.DefaultD3DUsage(), format, g_Windowing.DefaultD3DPool());
}

void CDXTexture::DestroyTextureObject()
{
  m_texture.Release();
}

void CDXTexture::LoadToGPU()
{
  if (!m_pixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }

  if (m_texture.Get() == NULL)
  {
    CreateTextureObject();
    if (m_texture.Get() == NULL)
    {
      CLog::Log(LOGDEBUG, "CDXTexture::CDXTexture: Error creating new texture for size %d x %d", m_textureWidth, m_textureHeight);
      return;
    }
  }

  D3DLOCKED_RECT lr;
  if (m_texture.LockRect( 0, &lr, NULL, D3DLOCK_DISCARD ))
  {
    unsigned char *dst = (unsigned char *)lr.pBits;
    unsigned char *src = m_pixels;
    unsigned int dstPitch = lr.Pitch;
    unsigned int srcPitch = GetPitch();
    unsigned int minPitch = std::min(srcPitch, dstPitch);

    unsigned int rows = GetRows();
    if (m_format == XB_FMT_RGB8)
    {
      for (unsigned int y = 0; y < rows; y++)
      {
        unsigned char *dst2 = dst;
        unsigned char *src2 = src;
        for (unsigned int x = 0; x < srcPitch / 3; x++, dst2 += 4, src2 += 3)
        {
          dst2[0] = src2[2];
          dst2[1] = src2[1];
          dst2[2] = src2[0];
          dst2[3] = 0xff;
        }
        src += srcPitch;
        dst += dstPitch;
      }
    }
    else if (srcPitch == dstPitch)
    {
      memcpy(dst, src, srcPitch * rows);
    }
    else
    {
      for (unsigned int y = 0; y < rows; y++)
      {
        memcpy(dst, src, minPitch);
        src += srcPitch;
        dst += dstPitch;
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock texture");
  }
  m_texture.UnlockRect(0);

  delete [] m_pixels;
  m_pixels = NULL;

  m_loadedToGPU = true;
}

void CDXTexture::BindToUnit(unsigned int unit)
{
  LPDIRECT3DDEVICE9 p3DDevice = g_Windowing.Get3DDevice();
  p3DDevice->SetTexture( unit, m_texture.Get() );
  p3DDevice->SetSamplerState( unit, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
  p3DDevice->SetSamplerState( unit, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
  p3DDevice->SetSamplerState( unit, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  p3DDevice->SetSamplerState( unit, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
}

#endif
