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
#include "TextureManager.h"
#include "Settings.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "Util.h"

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
  CBaseTexture* pTexture = NULL;
  //pTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, m_info.width, m_info.height, 32, RMASK, GMASK, BMASK, AMASK);
  pTexture = new CTexture(m_info.width, m_info.height, 32);
 
  if (pTexture)
  {
    //if (SDL_LockSurface(pTexture) == 0)
    //{
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
      //}
      //SDL_UnlockSurface(pTexture);
    }
  }
  else
    CLog::Log(LOGERROR, "%s - failed to create texture while loading image %s", __FUNCTION__, strFileName.c_str());
  m_dll.ReleaseImage(&m_info);
  
  return pTexture;
}

// caches a skin image as a thumbnail image
bool CPictureGL::CacheSkinImage(const CStdString &srcFile, const CStdString &destFile)
{
  /* elis
  int iImages = g_TextureManager.Load(srcFile);
  if (iImages > 0)
  {
    int width = 0, height = 0;
    bool linear = false;
    CTextureArray baseTexture = g_TextureManager.GetTexture(srcFile);
    XBMC::TexturePtr texture = NULL;
    if (texture)
    {
      bool success(false);
      CPicture pic;
      if (!linear)
      { // damn, have to copy it to a linear texture first :(
        return CreateThumbnailFromSwizzledTexture(texture, width, height, destFile);
      }
      else
      {
        SDL_LockSurface(texture);
        success = pic.CreateThumbnailFromSurface((BYTE *)texture->pixels, width, height, texture->pitch, destFile);
        SDL_UnlockSurface(texture);
      }
      g_TextureManager.ReleaseTexture(srcFile);
      return success;
    }
  }
  */
  return false;
}

bool CPictureGL::CreateThumbnailFromSwizzledTexture(CBaseTexture* &texture, int width, int height, const CStdString &thumb)
{
#ifdef __GNUC__
// FIXME: CPictureGL::CreateThumbnailFromSwizzledTexture not implemented
#endif
  return false;
}

#endif