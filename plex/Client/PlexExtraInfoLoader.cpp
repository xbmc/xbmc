#include "PlexExtraInfoLoader.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "DirectoryCache.h"
#include "PlexBusyIndicator.h"
#include "PlexApplication.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexExtraInfoLoader::CPlexExtraInfoLoader()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexExtraInfoLoader::~CPlexExtraInfoLoader()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraInfoLoader::LoadExtraInfoForItem(const CFileItemListPtr& list, const CFileItemPtr& extraItem, bool block)
{
  if (!list)
    return;

  std::string url;

  EPlexDirectoryType type = list->GetPlexDirectoryType();
  if (extraItem)
    type = extraItem->GetPlexDirectoryType();

  if (type == PLEX_DIR_TYPE_SEASON || type == PLEX_DIR_TYPE_EPISODE ||
      type == PLEX_DIR_TYPE_ALBUM || type == PLEX_DIR_TYPE_TRACK)
  {
    if (list->HasProperty("parentKey"))
      url = list->GetProperty("parentKey").asString();
    else if (extraItem && extraItem->HasProperty("parentKey"))
      url = extraItem->GetProperty("parentKey").asString();
    else if (!extraItem)
    {
      for (int i = 0; i < list->Size(); i++)
      {
        CFileItemPtr item = list->Get(i);
        if (item && item->HasProperty("parentKey"))
        {
          url = item->GetProperty("parentKey").asString();
          break;
        }
      }
    }
  }

  if (!url.empty())
  {
    CFileItemList cacheList;
    if (g_directoryCache.GetDirectory(url, cacheList))
    {
      CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::LoadExtraInfoForItem clean cache hit on %s",
                url.c_str());
      CopyProperties(list, cacheList.Get(0));
      return;
    }

    CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::LoadExtraInfoForItem loading %s for item %s",
              url.c_str(), list->GetPath().c_str());

    CPlexExtraInfoLoaderJob* job = new CPlexExtraInfoLoaderJob(CURL(url), list, block);
    if (block)
      g_plexApplication.busy.blockWaitingForJob(job, this);
    else
      CJobManager::GetInstance().AddJob(job, this, CJob::PRIORITY_HIGH);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraInfoLoader::CopyProperties(const CFileItemListPtr& list, CFileItemPtr extraItem)
{
  if (!list || !extraItem)
    return;

  if (extraItem->HasProperty("summary") && !extraItem->GetProperty("summary").empty() &&
      !list->HasProperty("showplot"))
  {
    list->SetProperty("showplot", extraItem->GetProperty("summary"));
    if (!list->HasProperty("summary"))
      list->SetProperty("summary", extraItem->GetProperty("summary"));
  }

  for (int i = 0; i < list->Size(); i++)
  {
    CFileItemPtr item = list->Get(i);

    /* copy Properties */
    std::pair<CStdString, CVariant> p;
    const PropertyMap pMap = extraItem->GetAllProperties();
    BOOST_FOREACH(p, pMap)
    {
      /* we only insert the properties if they are not available */
      if (!item->HasProperty(p.first))
        item->SetProperty(p.first, p.second);
    }

    /* copy Art */
    std::pair<CStdString, CStdString> sP;
    BOOST_FOREACH(sP, extraItem->GetArt())
    {
      if (!item->HasArt(sP.first))
        item->SetArt(sP.first, sP.second);
    }

    if (extraItem->HasVideoInfoTag())
    {
      CVideoInfoTag* infoTag = item->GetVideoInfoTag();
      CVideoInfoTag* infoTag2 = extraItem->GetVideoInfoTag();

      infoTag->m_genre.insert(infoTag->m_genre.end(), infoTag2->m_genre.begin(),
                              infoTag2->m_genre.end());
      infoTag->m_cast.insert(infoTag->m_cast.end(), infoTag2->m_cast.begin(),
                             infoTag2->m_cast.end());
      infoTag->m_writingCredits.insert(infoTag->m_writingCredits.end(),
                                       infoTag2->m_writingCredits.begin(),
                                       infoTag2->m_writingCredits.end());
      infoTag->m_director.insert(infoTag->m_director.end(), infoTag2->m_director.begin(),
                                 infoTag2->m_director.end());
    }
    else if (extraItem->HasMusicInfoTag())
    {
      MUSIC_INFO::CMusicInfoTag* musicInfoTag = item->GetMusicInfoTag();
      MUSIC_INFO::CMusicInfoTag* musicInfoTag2 = extraItem->GetMusicInfoTag();

      std::vector<std::string> genres = musicInfoTag2->GetGenre();
      std::vector<std::string> genres2 = musicInfoTag->GetGenre();
      BOOST_FOREACH(std::string g, genres)
      {
        if (std::find(genres2.begin(), genres2.end(), g) == genres2.end())
          genres2.push_back(g);
      }

      musicInfoTag->SetGenre(genres2);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraInfoLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{

  CPlexExtraInfoLoaderJob* fjob = static_cast<CPlexExtraInfoLoaderJob*>(job);
  if (!job)
    return;

  CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::OnJobComplete loaded extra info for %s, sucess: %s",
            fjob->m_extraList->GetPath().c_str(), success ? "YES" : "NO");

  CFileItemPtr extraItem = fjob->m_items.Get(0);

  if (extraItem)
  {
    CopyProperties(fjob->m_extraList, extraItem);
    LoadExtraInfoForItem(fjob->m_extraList, extraItem, fjob->m_block);
  }

  g_directoryCache.SetDirectory(fjob->m_url.Get(), fjob->m_items, XFILE::DIR_CACHE_ALWAYS);
}
