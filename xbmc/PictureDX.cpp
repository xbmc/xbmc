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
#include "PictureDX.h"
#include "TextureManager.h"
#include "Settings.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "Util.h"

#ifdef HAS_DX

using namespace XFILE;

CPictureDX::CPictureDX(void)
: CPictureBase()
{
  
}

CPictureDX::~CPictureDX(void)
{

}

CBaseTexture* CPictureDX::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
  CBaseTexture* pTexture = new CTexture(m_info.width,  m_info.height, 32);

  //g_graphicsContext.Get3DDevice()->CreateTexture(m_info.width, m_info.height, 1, 0, D3DFMT_LIN_A8R8G8B8 , D3DPOOL_MANAGED, &pTexture, NULL);

  if (pTexture)
  {
    D3DLOCKED_RECT lr;
    if ( D3D_OK == pTexture->GetTextureObject()->LockRect( 0, &lr, NULL, 0 ))
    {
      DWORD destPitch = lr.Pitch;
      // CxImage aligns rows to 4 byte boundaries
      DWORD srcPitch = ((m_info.width + 1)* 3 / 4) * 4;
      BYTE *pixels = (BYTE *)lr.pBits;

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
      pTexture->GetTextureObject()->UnlockRect( 0 );
    }
  }
  else
    CLog::Log(LOGERROR, "%s - failed to create texture while loading image %s", __FUNCTION__, strFileName.c_str());
  m_dll.ReleaseImage(&m_info);
  return pTexture;
}

// caches a skin image as a thumbnail image
bool CPictureDX::CacheSkinImage(const CStdString &srcFile, const CStdString &destFile)
{
  int iImages = g_TextureManager.Load(srcFile);
  if (iImages > 0)
  {
    int width = 0, height = 0;
    bool linear = false;
    CTextureArray baseTexture = g_TextureManager.GetTexture(srcFile);
    CBaseTexture* texture = baseTexture.m_textures[0];
    if (texture && texture->GetTextureObject())
    {
      bool success(false);
      CPicture pic;
      if (!linear)
      { // damn, have to copy it to a linear texture first :(
        return CreateThumbnailFromSwizzledTexture(texture, width, height, destFile);
      }
      else
      {
        D3DLOCKED_RECT lr;
        texture->GetTextureObject()->LockRect(0, &lr, NULL, 0);
        success = pic.CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, destFile);
        texture->GetTextureObject()->UnlockRect(0);
      }
      g_TextureManager.ReleaseTexture(srcFile);
      return success;
    }
  }
  return false;
}

bool CPictureDX::CreateThumbnailFromSwizzledTexture(CBaseTexture* &texture, int width, int height, const CStdString &thumb)
{
  LPDIRECT3DTEXTURE9 linTexture = NULL;
  if (D3D_OK == D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &linTexture))
  {
    LPDIRECT3DSURFACE9 source;
    LPDIRECT3DSURFACE9 dest;
    texture->GetTextureObject()->GetSurfaceLevel(0, &source);
    linTexture->GetSurfaceLevel(0, &dest);
    D3DXLoadSurfaceFromSurface(dest, NULL, NULL, source, NULL, NULL, D3DX_FILTER_NONE, 0);
    D3DLOCKED_RECT lr;
    dest->LockRect(&lr, NULL, 0);
    bool success = CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, thumb);
    dest->UnlockRect();
    SAFE_RELEASE(source);
    SAFE_RELEASE(dest);
    SAFE_RELEASE(linTexture);
    return success;
  }
  return false;
}

#endif
