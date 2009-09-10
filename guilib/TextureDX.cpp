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

#include "include.h"
#include "TextureDX.h"
#include "WindowingFactory.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"

#ifdef HAS_DX

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
CDXTexture::CDXTexture(unsigned int width, unsigned int height, unsigned int BPP)
: CBaseTexture(width, height, BPP)
{
  Allocate(m_imageWidth, m_imageHeight, m_nBPP);
}

CDXTexture::~CDXTexture()
{
  DestroyTextureObject();
}

void CDXTexture::CreateTextureObject()
{
  D3DFORMAT format;

  if (m_nBPP == 8)
    format = D3DFMT_LIN_A8;
  else if (m_nBPP == 32)
    format = D3DFMT_LIN_A8R8G8B8;
  else
    return;

  SAFE_RELEASE(m_pTexture);

  D3DXCreateTexture(g_Windowing.Get3DDevice(), m_nTextureWidth, m_nTextureHeight, 1, 0, format, D3DPOOL_MANAGED , &m_pTexture);
}

void CDXTexture::DestroyTextureObject()
{
  SAFE_RELEASE(m_pTexture);
}

void CDXTexture::LoadToGPU()
{
  if (!m_pPixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }

  if (m_pTexture == NULL)
  {
    CreateTextureObject();
    if (m_pTexture == NULL)
    {
      CLog::Log(LOGDEBUG, "CDXTexture::CDXTexture: Error creating new texture for size %d x %d", m_nTextureWidth, m_nTextureHeight);
      return;
    }
  }

  D3DLOCKED_RECT lr;
  if ( D3D_OK == m_pTexture->LockRect( 0, &lr, NULL, 0 ))
  {
    DWORD destPitch = lr.Pitch;
    
    DWORD srcPitch = m_imageWidth * m_nBPP / 8;
    BYTE *pixels = (BYTE *)lr.pBits;

    for (unsigned int y = 0; y < m_nTextureHeight; y++)
    {
      BYTE *dst = pixels + y * destPitch;
      BYTE *src = m_pPixels + y * srcPitch;
      memcpy(dst, src, srcPitch);
    }
  }
  m_pTexture->UnlockRect(0);

  delete [] m_pPixels;
  m_pPixels = NULL;

  m_loadedToGPU = true;   
}

#endif
