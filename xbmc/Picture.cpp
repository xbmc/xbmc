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

#include "Picture.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/FileCurl.h"
#include "Util.h"
#include "Texture.h"

using namespace XFILE;

CPicture::CPicture(void)
{
  ZeroMemory(&m_info, sizeof(ImageInfo));
}

CPicture::~CPicture(void)
{

}

bool CPicture::Load(const CStdString& strFileName, CBaseTexture* pTexture, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  if (pTexture == NULL)
    return false;

  memset(&m_info, 0, sizeof(ImageInfo));

  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return false;
  }

  pTexture->Allocate(m_info.width, m_info.height, XB_FMT_A8R8G8B8);

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
  {
    CLog::Log(LOGERROR, "%s - failed to create texture while loading image %s", __FUNCTION__, strFileName.c_str());
    return false;
  }
  
  m_dll.ReleaseImage(&m_info);

  return true;
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
  CURL url(strFileName);
  if (url.GetProtocol().Equals("http") || url.GetProtocol().Equals("https"))
  {
    CFileCurl http;
    CStdString thumbData;
    if (http.Get(strFileName, thumbData))
      return CreateThumbnailFromMemory((const BYTE *)thumbData.c_str(), thumbData.size(), CUtil::GetExtension(strFileName), strThumbFileName);
  }
  if (!m_dll.CreateThumbnail(strFileName.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "PICTURE::DoCreateThumbnail: Unable to create thumbfile %s from image %s", strThumbFileName.c_str(), strFileName.c_str());
    return false;
  }
  return true;
}

bool CPicture::CacheImage(const CStdString& sourceFileName, const CStdString& destFileName)
{
  CLog::Log(LOGINFO, "Caching image from: %s to %s", sourceFileName.c_str(), destFileName.c_str());

#ifdef RESAMPLE_CACHED_IMAGES
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnail(sourceFileName.c_str(), destFileName.c_str(), 1280, 720, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFileName.c_str(), sourceFileName.c_str());
    return false;
  }
  return true;
#else
  return CFile::Cache(sourceFileName, destFileName);
#endif
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
