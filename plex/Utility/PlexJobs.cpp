//
//  PlexJobs.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#include "PlexJobs.h"
#include "FileSystem/PlexDirectory.h"

#include "FileSystem/PlexFile.h"

#include "TextureCache.h"

////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPFetchJob::DoWork()
{
  return m_http.Get(m_url.Get(), m_data);
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPFetchJob::operator==(const CJob* job) const
{
  const CPlexHTTPFetchJob *f = static_cast<const CPlexHTTPFetchJob*>(job);
  return m_url.Get() == f->m_url.Get();
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectoryFetchJob::DoWork()
{
  return m_dir.GetDirectory(m_url.Get(), m_items);
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexMediaServerClientJob::DoWork()
{
  XFILE::CPlexFile file;
  bool success = false;
  
  if (m_verb == "PUT")
    success = file.Put(m_url.Get(), m_data);
  else if (m_verb == "GET")
    success = file.Get(m_url.Get(), m_data);
  else if (m_verb == "DELETE")
    success = file.Delete(m_url.Get(), m_data);
  else if (m_verb == "POST")
    success = file.Post(m_url.Get(), m_postData, m_data);
  
  return success;
}

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexVideoThumbLoaderJob::DoWork()
{
  if (!m_item->IsPlexMediaServer())
    return false;

  if (m_item->HasArt("thumb") &&
      !CTextureCache::Get().HasCachedImage(m_item->GetArt("thumb")))
    CTextureCache::Get().BackgroundCacheImage(m_item->GetArt("thumb"));

  if (m_item->HasArt("fanart") &&
      !CTextureCache::Get().HasCachedImage(m_item->GetArt("fanart")))
    CTextureCache::Get().BackgroundCacheImage(m_item->GetArt("fanart"));

  if (m_item->HasArt("grandParentThumb") &&
      !CTextureCache::Get().HasCachedImage(m_item->GetArt("grandParentThumb")))
    CTextureCache::Get().BackgroundCacheImage(m_item->GetArt("grandParentThumb"));

  if (m_item->HasArt("bigPoster") &&
      !CTextureCache::Get().HasCachedImage(m_item->GetArt("bigPoster")))
    CTextureCache::Get().BackgroundCacheImage(m_item->GetArt("bigPoster"));

  return true;
}
