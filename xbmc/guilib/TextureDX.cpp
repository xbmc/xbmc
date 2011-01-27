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
    if (srcPitch == dstPitch)
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

#endif
