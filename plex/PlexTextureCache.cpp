#include "URIUtils.h"
#include "PlexUtils.h"
#include "settings/Settings.h"
#include "PlexTextureCache.h"
#include "log.h"
#include "File.h"
#include "PlexJobs.h"

using namespace XFILE;

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTextureCache::Deinitialize()
{
  CancelJobs();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexTextureCache::HasCachedImage(const CStdString &url)
{
  CTextureDetails details;
  return GetCachedTexture(url,details);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexTextureCache::GetCachedImage(const CStdString &image, CTextureDetails &details, bool trackUsage)
{
  if (GetCachedTexture(image, details))
  {
    return GetCachedPath(details.file);
  }
  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexTextureCache::CheckCachedImage(const CStdString &url, bool returnDDS, bool &needsRecaching)
{
  CTextureDetails details;
  CStdString cachedImage = GetCachedImage(url, details);

  needsRecaching = (!cachedImage.IsEmpty());
  return cachedImage;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTextureCache::BackgroundCacheImage(const CStdString &url)
{
  CTextureDetails details;
  if (GetCachedTexture(url, details))
    return; // image is already cached and doesn't need to be checked further

  // needs (re)caching
  AddJob(new CPlexTextureCacheJob(UnwrapImageURL(url), details.hash));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTextureCache::ClearCachedImage(const CStdString &url, bool deleteSource /*= false */)
{
  CTextureDetails details;
  CStdString path = GetCachedImage(url, details);
  CFile::Delete(path);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexTextureCache::GetCachedTexture(const CStdString &url, CTextureDetails &details)
{
  CStdString fileprefix = CTextureCache::GetCacheFile(url);
  CStdString path = CTextureCache::GetCachedPath(fileprefix);

  // first check if we have a jpg matching
  if  (CFile::Exists(path + ".jpg"))
  {
    details.file = fileprefix + ".jpg";
    details.hash = fileprefix.Right(8);
    return true;
  }

  // if not we might have a png
  if  (CFile::Exists(path + ".png"))
  {
    details.file = fileprefix + ".png";
    details.hash = fileprefix.Right(8);
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexTextureCache::AddCachedTexture(const CStdString &url, const CTextureDetails &details)
{
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTextureCache::IncrementUseCount(const CTextureDetails &details)
{
  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexTextureCache::SetCachedTextureValid(const CStdString &url, bool updateable)
{
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTextureCache::OnCachingComplete(bool success, CTextureCacheJob *job)
{
  // remove from our processing list
  CSingleLock lock(m_processingSection);
  std::set<CStdString>::iterator i = m_processing.find(job->m_url);
  if (i != m_processing.end())
    m_processing.erase(i);

  m_completeEvent.Set();
}


