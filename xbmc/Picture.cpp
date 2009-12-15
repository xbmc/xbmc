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
#include "DllImageLib.h"

using namespace XFILE;

bool CPicture::CreateThumbnail(const CStdString& file, const CStdString& thumbFile, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(thumbFile))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", file.c_str(), thumbFile.c_str());

  CURL url(file);
  if (url.GetProtocol().Equals("http") || url.GetProtocol().Equals("https"))
  {
    CFileCurl http;
    CStdString thumbData;
    if (http.Get(file, thumbData))
      return CreateThumbnailFromMemory((const unsigned char *)thumbData.c_str(), thumbData.size(), CUtil::GetExtension(file), thumbFile);
  }
  // load our dll
  DllImageLib dll;
  if (!dll.Load()) return false;
  if (!dll.CreateThumbnail(file.c_str(), thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "%s: Unable to create thumbfile %s from image %s", __FUNCTION__, thumbFile.c_str(), file.c_str());
    return false;
  }
  return true;
}

bool CPicture::CacheImage(const CStdString& sourceFile, const CStdString& destFile)
{
  CLog::Log(LOGINFO, "Caching image from: %s to %s", sourceFile.c_str(), destFile.c_str());

#ifdef RESAMPLE_CACHED_IMAGES
  DllImageLib dll;
  if (!dll.Load()) return false;
  if (!dll.CreateThumbnail(sourceFile.c_str(), destFile.c_str(), 1280, 720, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFilec_str(), sourceFile.c_str());
    return false;
  }
  return true;
#else
  return CFile::Cache(sourceFile, destFile);
#endif
}

bool CPicture::CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", thumbFile.c_str());
  DllImageLib dll;
  if (!dll.Load()) return false;
  if (!dll.CreateThumbnailFromMemory((BYTE *)buffer, bufSize, extension.c_str(), thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s: exception with fileType: %s", __FUNCTION__, extension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *thumbs, const CStdString &folderThumb)
{ // we want to mold the thumbs together into one single one
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
  DllImageLib dll;
  if (!dll.Load()) return;
  if (!dll.CreateFolderThumbnail(szThumbs, folderThumb.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s failed for folder thumb %s", __FUNCTION__, folderThumb.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(const unsigned char *buffer, int width, int height, int stride, const CStdString &thumbFile)
{
  DllImageLib dll;
  if (!buffer || !dll.Load()) return false;
  return dll.CreateThumbnailFromSurface((BYTE *)buffer, width, height, stride, thumbFile.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror)
{
  DllImageLib dll;
  if (!dll.Load()) return false;
  int ret = dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality, mirror);
  if (ret)
  {
    CLog::Log(LOGERROR, "%s: Error %i converting image %s", __FUNCTION__, ret, srcFile.c_str());
    return ret;
  }
  return ret;
}

CThumbnailWriter::CThumbnailWriter(unsigned char* buffer, int width, int height, int stride, const CStdString& thumbFile)
{
  m_buffer    = buffer;
  m_width     = width;
  m_height    = height;
  m_stride    = stride;
  m_thumbFile = thumbFile;
}

bool CThumbnailWriter::DoWork()
{
  if (!CPicture::CreateThumbnailFromSurface(m_buffer, m_width, m_height, m_stride, m_thumbFile))
    CLog::Log(LOGERROR, "Unable to write screenshot %s", m_thumbFile.c_str());

  delete m_buffer;
}

