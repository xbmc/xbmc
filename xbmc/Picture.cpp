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
#include "Picture.h"
#include "TextureManager.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "Util.h"

using namespace XFILE;

CPicture::CPicture(void)
{
  ZeroMemory(&m_info, sizeof(ImageInfo));
}

CPicture::~CPicture(void)
{

}

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
  LPDIRECT3DTEXTURE8 pTexture = NULL;
  g_graphicsContext.Get3DDevice()->CreateTexture(m_info.width, m_info.height, 1, 0, D3DFMT_LIN_A8R8G8B8 , D3DPOOL_MANAGED, &pTexture);
  if (pTexture)
  {
    D3DLOCKED_RECT lr;
    if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
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
      pTexture->UnlockRect( 0 );
    }
  }
  else
    CLog::Log(LOGERROR, "%s - failed to create texture while loading image %s", __FUNCTION__, strFileName.c_str());
  m_dll.ReleaseImage(&m_info);
  return pTexture;
}

bool CPicture::CreateThumbnail(const CStdString& file, const CStdString& thumbFile, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(thumbFile))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", file.c_str(), thumbFile.c_str());

  memset(&m_info, 0, sizeof(ImageInfo));
  if (CUtil::IsInternetStream(file, true))
  {
    CFileCurl stream;
    CStdString thumbData;
    if (stream.Get(file, thumbData))
      return CreateThumbnailFromMemory((const BYTE *)thumbData.c_str(), thumbData.size(), CUtil::GetExtension(file), thumbFile);

    return false;
  }      
  // load our dll
  if (!m_dll.Load()) return false;

  if (!m_dll.CreateThumbnail(file.c_str(), thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "%s: Unable to create thumbfile %s from image %s", __FUNCTION__, thumbFile.c_str(), file.c_str());
    return false;
  }
  return true;
}

bool CPicture::CacheImage(const CStdString& sourceUrl, const CStdString& destFile, int width, int height)
{
  if (width > 0 && height > 0)
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s with width %i and height %i", sourceUrl.c_str(), destFile.c_str(), width, height);
    if (!m_dll.Load()) return false;
  
    if (CUtil::IsInternetStream(sourceUrl, true))
    {
      CFileCurl stream;
      CStdString tempFile = "special://temp/image_download.jpg";
      if (stream.Download(sourceUrl, tempFile))
      {
        if (!m_dll.CreateThumbnail(tempFile.c_str(), destFile.c_str(), width, height, g_guiSettings.GetBool("pictures.useexifrotation")))
        {
          CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
          CFile::Delete(tempFile);
          return false;
        }
        CFile::Delete(tempFile);
        return true;
      }
      return false;
    }
    
    if (!m_dll.CreateThumbnail(sourceUrl.c_str(), destFile.c_str(), width, height, g_guiSettings.GetBool("pictures.useexifrotation")))
    {
      CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
      return false;
    }
    return true;
  }
  else
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s", sourceUrl.c_str(), destFile.c_str());
    if (CUtil::IsInternetStream(sourceUrl, true))
    {
      CFileCurl stream;
      return stream.Download(sourceUrl, destFile);
    }
    else
    {
      return CFile::Cache(sourceUrl, destFile);
    }
  }
}

bool CPicture::CacheThumb(const CStdString& sourceUrl, const CStdString& destFile)
{
  return CacheImage(sourceUrl, destFile, g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize);
}

bool CPicture::CacheFanart(const CStdString& sourceUrl, const CStdString& destFile)
{
  int height = g_advancedSettings.m_fanartHeight;
  // Assume 16:9 size
  int width = height * 16 / 9;

  return CacheImage(sourceUrl, destFile, width, height);
}

bool CPicture::CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", thumbFile.c_str());
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnailFromMemory((BYTE *)buffer, bufSize, extension.c_str(), thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s: exception with fileType: %s", __FUNCTION__, extension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *thumbs, const CStdString &folderThumb)
{ // we want to mold the thumbs together into one single one
  if (!m_dll.Load()) return;
  CStdString cachedThumbs[4];
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
  {
    if (!thumbs[i].IsEmpty())
    {
      CFileItem item(thumbs[i], false);
      cachedThumbs[i] = item.GetCachedPictureThumb();
      CreateThumbnail(thumbs[i], cachedThumbs[i], true);
    }
    szThumbs[i] = cachedThumbs[i].c_str();
  }
  if (!m_dll.CreateFolderThumbnail(szThumbs, folderThumb.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s failed for folder thumb %s", __FUNCTION__, folderThumb.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName)
{
  if (!pBuffer || !m_dll.Load()) return false;
  return m_dll.CreateThumbnailFromSurface(pBuffer, width, height, stride, strThumbFileName.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror)
{
  if (!m_dll.Load()) return false;
  int ret;
  ret=m_dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality, mirror);
  if (ret!=0)
  {
    CLog::Log(LOGERROR, "PICTURE: Error %i converting image %s", ret, srcFile.c_str());
    return ret;
  }
  return ret;
}

// caches a skin image as a thumbnail image
bool CPicture::CacheSkinImage(const CStdString &srcFile, const CStdString &destFile)
{
  int iImages = g_TextureManager.Load(srcFile);
  if (iImages > 0)
  {
    CTexture texture = g_TextureManager.GetTexture(srcFile);
    if (texture.size())
    {
      bool success(false);
      CPicture pic;
      if (!texture.m_texCoordsArePixels)
      { // damn, have to copy it to a linear texture first :(
        return CreateThumbnailFromSwizzledTexture(texture.m_textures[0], texture.m_width, texture.m_height, destFile);
      }
      else
      {
        D3DLOCKED_RECT lr;
        texture.m_textures[0]->LockRect(0, &lr, NULL, 0);
        success = pic.CreateThumbnailFromSurface((BYTE *)lr.pBits, texture.m_width, texture.m_height, lr.Pitch, destFile);
        texture.m_textures[0]->UnlockRect(0);
      }
      g_TextureManager.ReleaseTexture(srcFile);
      return success;
    }
  }
  return false;
}

bool CPicture::CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumb)
{
  LPDIRECT3DTEXTURE8 linTexture = NULL;
  if (D3D_OK == D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &linTexture))
  {
    LPDIRECT3DSURFACE8 source;
    LPDIRECT3DSURFACE8 dest;
    texture->GetSurfaceLevel(0, &source);
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

