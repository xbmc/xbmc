/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "TextureCache.h"
#include "FileSystem/File.h"
#include "utils/SingleLock.h"
#include "Crc32.h"
#include "Util.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "utils/log.h"

#include "Texture.h"
#include "DDSImage.h"
#include "TextureManager.h"

using namespace XFILE;

CTextureCache::CDDSJob::CDDSJob(const CStdString &original)
{
  m_original = original;
}

bool CTextureCache::CDDSJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CDDSJob* ddsJob = dynamic_cast<const CDDSJob*>(job);
    if (ddsJob && ddsJob->m_original == m_original)
      return true;
  }
  return false;
}

bool CTextureCache::CDDSJob::DoWork()
{
  CTexture texture;
  if (texture.LoadFromFile(m_original))
  { // convert to DDS
    CDDSImage dds;
    CLog::Log(LOGDEBUG, "Creating DDS version of: %s", m_original.c_str()); 
    if (dds.Compress(texture.GetWidth(), texture.GetHeight(), texture.GetPitch(), texture.GetPixels(), 40))
    {
      CStdString ddsFile = CUtil::ReplaceExtension(m_original, ".dds");
      dds.WriteFile(ddsFile);
      return true;
    }
  }
  return false;
}

CTextureCache &CTextureCache::Get()
{
  static CTextureCache s_cache;
  return s_cache;
}

CTextureCache::CTextureCache()
{
}

CTextureCache::~CTextureCache()
{
}

void CTextureCache::Initialize()
{
  CSingleLock lock(m_databaseSection);
  if (!m_database.IsOpen())
    m_database.Open();
}

void CTextureCache::Deinitialize()
{
  CancelJobs();
  CSingleLock lock(m_databaseSection);
  m_database.Close();
}

bool CTextureCache::IsCachedImage(const CStdString &url) const
{
  if (0 == strncmp(url.c_str(), "special://skin/", 15)) // a skin image
    return true;
  CStdString basePath(g_settings.GetThumbnailsFolder());
  if (0 == strncmp(url.c_str(), basePath.c_str(), basePath.GetLength()))
    return true;
  return g_TextureManager.CanLoad(url);
}

CStdString CTextureCache::GetCachedImage(const CStdString &url)
{
  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  CStdString cacheFile;
  if (GetCachedTexture(url, cacheFile))
    return GetCachedPath(cacheFile);
  return "";
}

CStdString CTextureCache::CheckAndCacheImage(const CStdString &url)
{
  CStdString path(GetCachedImage(url));
  if (!path.IsEmpty())
  {
    if (0 != strncmp(url.c_str(), "special://skin/", 15)) // TODO: should skin images be .dds'd (currently they're not necessarily writeable)
    { // check for dds version
      CStdString ddsPath = CUtil::ReplaceExtension(path, ".dds");
      if (CFile::Exists(ddsPath))
        return ddsPath;
      if (g_advancedSettings.m_useDDSFanart)
        AddJob(new CDDSJob(path));
    }
    return path;
  }

  // Cache the texture
  CStdString originalFile = GetCacheFile(url);
  CStdString originalURL = GetCachedPath(originalFile);

  CStdString hash = GetImageHash(url);
  if (!hash.IsEmpty() && CFile::Cache(url, originalURL))
  {
    AddCachedTexture(url, originalFile, hash);
    AddJob(new CDDSJob(originalFile));
    return originalURL;
  }

  return "";
}

void CTextureCache::ClearCachedImage(const CStdString &url)
{
  CStdString cachedFile;
  if (ClearCachedTexture(url, cachedFile))
  {
    CStdString path = GetCachedPath(cachedFile);
    if (CFile::Exists(path))
      CFile::Delete(path);
    path = CUtil::ReplaceExtension(path, ".dds");
    if (CFile::Exists(path))
      CFile::Delete(path);
  }
}

bool CTextureCache::GetCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.GetCachedTexture(url, cachedURL);
}

bool CTextureCache::AddCachedTexture(const CStdString &url, const CStdString &cachedURL, const CStdString &hash)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, cachedURL, hash);
}

bool CTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

CStdString CTextureCache::GetImageHash(const CStdString &url) const
{
  // TODO: stat the image and grab ctime/mtime and size
  return "nohash";
}

CStdString CTextureCache::GetCacheFile(const CStdString &url)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("%c/%s%s", hex[0], hex.c_str(), CUtil::GetExtension(url).c_str());
  return hash;
}

CStdString CTextureCache::GetCachedPath(const CStdString &file)
{
  return CUtil::AddFileToFolder(g_settings.GetThumbnailsFolder(), file);
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  return CJobQueue::OnJobComplete(jobID, success, job);
}
