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
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/FileCurl.h"
#include "DllImageLib.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

/* PLEX */
#include <boost/lexical_cast.hpp>
#include "sha.h"
#include "threads/Thread.h"
#include "PlexUtils.h"
/* END PLEX */

using namespace XFILE;

bool CPicture::CreateThumbnail(const CStdString& file, const CStdString& thumbFile, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(thumbFile))
    return true;

  return CacheImage(file, thumbFile, g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize);
}

#ifndef __PLEX__
bool CPicture::CacheImage(const CStdString& sourceUrl, const CStdString& destFile, int width, int height)
{
  if (width > 0 && height > 0)
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s with width %i and height %i", sourceUrl.c_str(), destFile.c_str(), width, height);
    
    DllImageLib dll;
    if (!dll.Load()) return false;

    if (URIUtils::IsInternetStream(sourceUrl, true))
    {
      CFileCurl http;
      CStdString data;
      if (http.Get(sourceUrl, data))
      {
        if (!dll.CreateThumbnailFromMemory((BYTE *)data.c_str(), data.GetLength(), URIUtils::GetExtension(sourceUrl).c_str(), destFile.c_str(), width, height))
        {
          CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
          return false;
        }
        return true;
      }
      return false;
    }

    if (!dll.CreateThumbnail(sourceUrl.c_str(), destFile.c_str(), width, height, g_guiSettings.GetBool("pictures.useexifrotation")))
    {
      CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
      return false;
    }
    return true;
  }
  else
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s", sourceUrl.c_str(), destFile.c_str());
    return CFile::Cache(sourceUrl, destFile);
  }
}
#else
bool CPicture::CacheImage(const CStdString& sourceUrl, const CStdString& destFile, int width, int height)
{
  // Short circuit if the file already exists.
  if (CFile::Exists(destFile))
    return true;

  CStdString tmpFile = destFile + ".tmp." + boost::lexical_cast<CStdString>(CThread::GetCurrentThreadId());
  if (GetMediaFromPlexMediaServerCache(sourceUrl, tmpFile) == false)
  {
    if (width > 0 && height > 0)
    {
      CLog::Log(LOGINFO, "Caching image from: %s to %s with width %i and height %i", sourceUrl.c_str(), destFile.c_str(), width, height);

      DllImageLib dll;
      if (!dll.Load()) return false;

      if (URIUtils::IsInternetStream(sourceUrl, true))
      {
        CFileCurl http;
        CStdString data;
        if (http.Get(sourceUrl, data))
        {
          if (!dll.CreateThumbnailFromMemory((BYTE *)data.c_str(), data.GetLength(), URIUtils::GetExtension(sourceUrl).c_str(), tmpFile.c_str(), width, height))
          {
            CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
            return false;
          }
          return true;
        }
        return false;
      }

      if (!dll.CreateThumbnail(sourceUrl.c_str(), tmpFile.c_str(), width, height, g_guiSettings.GetBool("pictures.useexifrotation")))
      {
        CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
        return false;
      }
      return true;
    }
  }

  // Atomically rename.
  if (CFile::Exists(tmpFile))
    CFile::Rename(tmpFile, destFile);

  return true;
}
#endif

bool CPicture::CacheThumb(const CStdString& sourceUrl, const CStdString& destFile)
{
  return CacheImage(sourceUrl, destFile, g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize);
}

bool CPicture::CacheFanart(const CStdString& sourceUrl, const CStdString& destFile)
{
  int height = g_advancedSettings.m_fanartHeight;
  /* PLEX */
  if (height == 0)
    height = 720;
  /* END PLEX */
  // Assume 16:9 size
  int width = height * 16 / 9;

  return CacheImage(sourceUrl, destFile, width, height);
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
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
    szThumbs[i] = thumbs[i].c_str();

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
  bool success = true;

  if (!CPicture::CreateThumbnailFromSurface(m_buffer, m_width, m_height, m_stride, m_thumbFile))
  {
    CLog::Log(LOGERROR, "CThumbnailWriter::DoWork unable to write %s", m_thumbFile.c_str());
    success = false;
  }

  delete [] m_buffer;

  return success;
}

/* PLEX */
bool CPicture::CacheBanner(const CStdString& sourceUrl, const CStdString& destFile)
{
  return CacheImage(sourceUrl, destFile, 0, 0);
}

bool CPicture::GetMediaFromPlexMediaServerCache(const CStdString& strFileName, const CStdString& strThumbFileName)
{
  CFileItem fileItem(strFileName, false);
  if (fileItem.IsPlexMediaServer() && (strFileName.Find("/photo/:/transcode") != -1))
  {
    CLog::Log(LOGDEBUG, "Asked to check media from PMS: %s", strFileName.c_str());

    // First optimize by checking for the actual cached file; size requested is always 720p.
    int start = fileItem.GetPath().Find("url=") + 4;
    CStdString url = fileItem.GetPath().substr(start);
    CURL::Decode(url);

    if (url.Find("127.0.0.1") != -1)
    {
      // Bogus, needs to match PlexDirectory.
      CStdString size = "1280-720";
      if (url.Find("/poster") != -1 || url.Find("/thumb") != -1)
        size = boost::lexical_cast<std::string>(g_advancedSettings.m_thumbSize) + "-" + boost::lexical_cast<std::string>(g_advancedSettings.m_thumbSize);
      else if (url.Find("/banner") != -1)
        size = "800-200";

      std::string cacheToken = url + "-" + size;

      // Compute SHA.
      SHA_CTX m_ctx;
      SHA1_Init(&m_ctx);
      SHA1_Update(&m_ctx, (const u_int8_t* )cacheToken.c_str(), cacheToken.size());
      char sha[SHA1_DIGEST_STRING_LENGTH];
      SHA1_End(&m_ctx, sha);
      std::string hash = sha;

      // Compute cache file.
      CStdString cacheFile = getenv("HOME");
      cacheFile += "/Library/Caches/PlexMediaServer/PhotoTranscoder/";
      cacheFile += hash.substr(0, 2) + "/";
      cacheFile += hash + ".jpg";

      // Copy it over
      if (CFile::Exists(cacheFile))
        return CFile::Cache(cacheFile, strThumbFileName);

      // Otherwise, let's take exactly what we get from the Plex Media Server.
      CLog::Log(LOGINFO, "Cache file didn't exist for %s (%s)", url.c_str(), cacheFile.c_str());
    }
  }

  CFile::Cache(strFileName, strThumbFileName, 0);
  if (PlexUtils::Size(strThumbFileName) == 0)
  {
    CFile::Delete(strThumbFileName);
    return false;
  }

  return true;
}
/* END PLEX */
