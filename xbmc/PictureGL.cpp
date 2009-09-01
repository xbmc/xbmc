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

#include "stdafx.h"
#include "PictureGL.h"
#include "Texture.h"

#ifdef HAS_GL

using namespace XFILE;

CPictureGL::CPictureGL(void) 
:CPictureBase()
{
  
}

CPictureGL::~CPictureGL(void)
{

}

CBaseTexture* CPictureGL::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
  CBaseTexture *pTexture = new CTexture(m_info.width, m_info.height, 32);
  if (pTexture)
  {
    DWORD destPitch = pTexture->GetPitch();
    DWORD srcPitch = ((m_info.width + 1)* 3 / 4) * 4;
    BYTE *pixels = (BYTE *)pTexture->GetPixels();
    for (unsigned int y = 0; y < m_info.height; y++)
    {
      BYTE *dst = pixels + y * destPitch;
      BYTE *src = m_info.texture + (m_info.height - 1 - y) * srcPitch;
      BYTE *alpha = m_info.alpha + (m_info.height - 1 - y) * m_info.width;
      for (unsigned int x = 0; x < m_info.width; x++)
      {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = (m_info.alpha) ? *alpha++ : 0xff;  // alpha
      }
    }
  }
  else
    CLog::Log(LOGERROR, "%s - failed to create texture while loading image %s", __FUNCTION__, strFileName.c_str());
  m_dll.ReleaseImage(&m_info);
  
  return pTexture;
}

#endif
