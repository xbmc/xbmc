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
#include "TextureCacheJob.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "URL.h"

using namespace XFILE;

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
  if (url != "-" && !CURL::IsFullPath(url))
    return true;
  if (URIUtils::IsInPath(url, "special://skin/") ||
      URIUtils::IsInPath(url, g_settings.GetThumbnailsFolder()))
    return true;
  return false;
}

bool CTextureCache::HasCachedImage(const CStdString &url)
{
  CStdString cachedHash;
  CStdString cachedImage(GetCachedImage(url, cachedHash));
  return (!cachedImage.IsEmpty() && cachedImage != url);
}

CStdString CTextureCache::GetCachedImage(const CStdString &image, CStdString &cachedHash, bool trackUsage)
{
  cachedHash.clear();
  CStdString url = UnwrapImageURL(image);

  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  CTextureDetails details;
  if (GetCachedTexture(url, details))
  {
    if (trackUsage)
      IncrementUseCount(details);
    return GetCachedPath(details.file);
  }
  return "";
}

CStdString CTextureCache::GetWrappedImageURL(const CStdString &image, const CStdString &type, const CStdString &options)
{
  if (image.compare(0, 8, "image://") == 0)
    return image; // already wrapped

  CStdString encoded(image);
  CURL::Encode(encoded);
  CStdString url = "image://";
  if (!type.IsEmpty())
    url += type + "@";
  url += encoded;
  if (!options.IsEmpty())
    url += "/transform?" + options;
  return url;
}

CStdString CTextureCache::GetWrappedThumbURL(const CStdString &image)
{
  return GetWrappedImageURL(image, "", "size=thumb");
}

CStdString CTextureCache::UnwrapImageURL(const CStdString &image)
{
  if (image.compare(0, 8, "image://") == 0)
  {
    CURL url(image);
    if (url.GetUserName().IsEmpty() && url.GetOptions().IsEmpty())
    {
      CStdString file(url.GetHostName());
      CURL::Decode(file);
      return file;
    }
  }
  return image;
}

CStdString CTextureCache::CheckCachedImage(const CStdString &url, bool returnDDS, bool &needsRecaching)
{
  CStdString cachedHash;
  CStdString path(GetCachedImage(url, cachedHash, true));
  needsRecaching = !cachedHash.IsEmpty();
  if (!path.IsEmpty())
  {
    if (!needsRecaching && returnDDS && !URIUtils::IsInPath(url, "special://skin/")) // TODO: should skin images be .dds'd (currently they're not necessarily writeable)
    { // check for dds version
      CStdString ddsPath = URIUtils::ReplaceExtension(path, ".dds");
      if (CFile::Exists(ddsPath))
        return ddsPath;
      if (g_advancedSettings.m_useDDSFanart)
        AddJob(new CTextureDDSJob(path));
    }
    return path;
  }
  return "";
}

void CTextureCache::BackgroundCacheImage(const CStdString &url)
{
  CStdString cacheHash;
  CStdString path(GetCachedImage(url, cacheHash));
  if (!path.IsEmpty() && cacheHash.IsEmpty())
    return; // image is already cached and doesn't need to be checked further

  // needs (re)caching
  AddJob(new CTextureCacheJob(UnwrapImageURL(url), cacheHash));
}

CStdString CTextureCache::CacheImage(const CStdString &image, CBaseTexture **texture)
{
  CStdString url = UnwrapImageURL(image);
  CSingleLock lock(m_processingSection);
  if (m_processing.find(url) == m_processing.end())
  {
    m_processing.insert(url);
    lock.Leave();
    // cache the texture directly
    CTextureCacheJob job(url);
    bool success = job.CacheTexture(texture);
    OnCachingComplete(success, &job);
    return success ? GetCachedPath(job.m_details.file) : "";
  }
  lock.Leave();

  // wait for currently processing job to end.
  while (true)
  {
    m_completeEvent.WaitMSec(1000);
    {
      CSingleLock lock(m_processingSection);
      if (m_processing.find(url) == m_processing.end())
        break;
    }
  }
  CStdString cachedHash;
  return GetCachedImage(url, cachedHash, true);
}

void CTextureCache::ClearCachedImage(const CStdString &url, bool deleteSource /*= false */)
{
  // TODO: This can be removed when the texture cache covers everything.
  CStdString path = deleteSource ? url : "";
  CStdString cachedFile;
  if (ClearCachedTexture(url, cachedFile))
    path = GetCachedPath(cachedFile);
  if (CFile::Exists(path))
    CFile::Delete(path);
  path = URIUtils::ReplaceExtension(path, ".dds");
  if (CFile::Exists(path))
    CFile::Delete(path);
}

bool CTextureCache::GetCachedTexture(const CStdString &url, CTextureDetails &details)
{
  CSingleLock lock(m_databaseSection);
  return m_database.GetCachedTexture(url, details);
}

bool CTextureCache::AddCachedTexture(const CStdString &url, const CTextureDetails &details)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, details);
}

void CTextureCache::IncrementUseCount(const CTextureDetails &details)
{
  static const size_t count_before_update = 100;
  CSingleLock lock(m_useCountSection);
  m_useCounts.reserve(count_before_update);
  m_useCounts.push_back(details);
  if (m_useCounts.size() >= count_before_update)
  {
    AddJob(new CTextureUseCountJob(m_useCounts));
    m_useCounts.clear();
  }
}

bool CTextureCache::SetCachedTextureValid(const CStdString &url, bool updateable)
{
  CSingleLock lock(m_databaseSection);
  return m_database.SetCachedTextureValid(url, updateable);
}

bool CTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

CStdString CTextureCache::GetCacheFile(const CStdString &url)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("%c/%s", hex[0], hex.c_str());
  return hash;
}

CStdString CTextureCache::GetCachedPath(const CStdString &file)
{
  return URIUtils::AddFileToFolder(g_settings.GetThumbnailsFolder(), file);
}

void CTextureCache::OnCachingComplete(bool success, CTextureCacheJob *job)
{
  if (success)
  {
    if (job->m_oldHash == job->m_details.hash)
      SetCachedTextureValid(job->m_url, job->m_details.updateable);
    else
      AddCachedTexture(job->m_url, job->m_details);
  }

  { // remove from our processing list
    CSingleLock lock(m_processingSection);
    std::set<CStdString>::iterator i = m_processing.find(job->m_url);
    if (i != m_processing.end())
      m_processing.erase(i);
  }

  m_completeEvent.Set();

  // TODO: call back to the UI indicating that it can update it's image...
  if (success && g_advancedSettings.m_useDDSFanart && !job->m_details.file.empty())
    AddJob(new CTextureDDSJob(GetCachedPath(job->m_details.file)));
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (strcmp(job->GetType(), "cacheimage") == 0)
    OnCachingComplete(success, (CTextureCacheJob *)job);
  return CJobQueue::OnJobComplete(jobID, success, job);
}

void CTextureCache::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  if (strcmp(job->GetType(), "cacheimage") == 0 && !progress)
  { // check our processing list
    {
      CSingleLock lock(m_processingSection);
      const CTextureCacheJob *cacheJob = (CTextureCacheJob *)job;
      std::set<CStdString>::iterator i = m_processing.find(cacheJob->m_url);
      if (i == m_processing.end())
      {
        m_processing.insert(cacheJob->m_url);
        return;
      }
    }
    CancelJob(job);
  }
  else
    CJobQueue::OnJobProgress(jobID, progress, total, job);
}

bool CTextureCache::Export(const CStdString &image, const CStdString &destination)
{
  CStdString cachedHash;
  CStdString cachedImage(GetCachedImage(image, cachedHash));
  if (!cachedImage.IsEmpty())
  {
    if (CFile::Cache(cachedImage, destination))
      return true;
    CLog::Log(LOGERROR, "%s failed exporting '%s' to '%s'", __FUNCTION__, cachedImage.c_str(), destination.c_str());
  }
  return false;
}
