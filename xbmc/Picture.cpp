/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "picture.h"
#include "util.h"
#include "TextureManager.h"


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
  // loaded successfully
  return m_info.texture;
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(strThumbFileName))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", strFileName.c_str(),strThumbFileName.c_str());

  // load our dll
  if (!m_dll.Load()) return false;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.CreateThumbnail(strFileName.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "PICTURE::DoCreateThumbnail: Unable to create thumbfile %s from image %s", strThumbFileName.c_str(), strFileName.c_str());
    return false;
  }
  return true;
}

bool CPicture::CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", strThumbFileName.c_str());
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnailFromMemory((BYTE *)pBuffer, nBufSize, strExtension.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateAlbumThumbnailFromMemory: exception: memfile FileType: %s\n", strExtension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail)
{ // we want to mold the thumbs together into one single one
  if (!m_dll.Load()) return;
  CStdString strThumbnails[4];
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
  {
    if (strThumbs[i].IsEmpty())
      strThumbnails[i] = "";
    else
    {
      CFileItem item(strThumbs[i], false);
      strThumbnails[i] = item.GetCachedPictureThumb();
      DoCreateThumbnail(strThumbs[i], strThumbnails[i], true);
    }
    szThumbs[i] = strThumbnails[i].c_str();
  }
  if (!m_dll.CreateFolderThumbnail(szThumbs, folderThumbnail.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateFolderThumb() failed for folder thumb %s", folderThumbnail.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName)
{
  if (!pBuffer || !m_dll.Load()) return false;
  return m_dll.CreateThumbnailFromSurface(pBuffer, width, height, stride, strThumbFileName.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality)
{
  if (!m_dll.Load()) return false;
  int ret;
  ret=m_dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality);
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
  int iImages = g_TextureManager.Load(srcFile, 0);
  if (iImages > 0)
  {
    int width, height;
    bool linear;
    LPDIRECT3DPALETTE8 palette;
    LPDIRECT3DTEXTURE8 texture = g_TextureManager.GetTexture(srcFile, 0, width, height, palette, linear);
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
        D3DLOCKED_RECT lr;
        texture->LockRect(0, &lr, NULL, 0);
        success = pic.CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, destFile);
        texture->UnlockRect(0);
      }
      g_TextureManager.ReleaseTexture(srcFile, 0);
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