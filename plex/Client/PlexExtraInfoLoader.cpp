#include "PlexExtraInfoLoader.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "DirectoryCache.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexExtraInfoLoader::CPlexExtraInfoLoader()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexExtraInfoLoader::~CPlexExtraInfoLoader()
{
  std::pair<int, CFileItemList*> p;
  BOOST_FOREACH(p, m_jobMap)
    CJobManager::GetInstance().CancelJob(p.first);
  m_jobMap.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraInfoLoader::LoadExtraInfoForItem(CFileItemList* list, CFileItemPtr extraItem)
{
  if (!list)
    return;

  std::string url;

  EPlexDirectoryType type = list->GetPlexDirectoryType();
  if (extraItem)
    type = extraItem->GetPlexDirectoryType();

  if (type == PLEX_DIR_TYPE_SEASON ||
      type == PLEX_DIR_TYPE_EPISODE ||
      type == PLEX_DIR_TYPE_ALBUM ||
      type == PLEX_DIR_TYPE_TRACK)
  {
    if (list->HasProperty("parentKey"))
      url = list->GetProperty("parentKey").asString();
    else if (extraItem && extraItem->HasProperty("parentKey"))
      url = extraItem->GetProperty("parentKey").asString();
    else if (!extraItem)
    {
      for (int i = 0; i < list->Size(); i ++)
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
      CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::LoadExtraInfoForItem clean cache hit on %s", url.c_str());
      CopyProperties(list, cacheList.Get(0));
      return;
    }

    int id = CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(CURL(url)), this, CJob::PRIORITY_HIGH);
    if (id != 0)
    {
      CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::LoadExtraInfoForItem loading %s for item %s", url.c_str(), list->GetPath().c_str());
      CSingleLock lk(m_lock);
      m_jobMap[id] = list;
    }
    else
    {
      CLog::Log(LOGERROR, "CPlexExtraInfoLoader::LoadExtraInfoForItem failed to create CPlexDirectoryFetchJob for url %s", url.c_str());
      return;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraInfoLoader::CopyProperties(CFileItemList* list, CFileItemPtr extraItem)
{
  if (!list || !extraItem)
    return;

  if (extraItem->HasProperty("summary") && !extraItem->GetProperty("summary").empty() && !list->HasProperty("showplot"))
  {
    list->SetProperty("showplot", extraItem->GetProperty("summary"));
    if (!list->HasProperty("summary"))
      list->SetProperty("summary", extraItem->GetProperty("summary"));
  }

  for (int i = 0; i < list->Size(); i ++)
  {
    CFileItemPtr item = list->Get(i);

    /* copy Properties */
    std::pair<CStdString, CVariant> p;
    const boost::unordered_map<CStdString, CVariant> pMap = extraItem->GetAllProperties();
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

      infoTag->m_genre.insert(infoTag->m_genre.end(), infoTag2->m_genre.begin(), infoTag2->m_genre.end());
      infoTag->m_cast.insert(infoTag->m_cast.end(), infoTag2->m_cast.begin(), infoTag2->m_cast.end());
      infoTag->m_writingCredits.insert(infoTag->m_writingCredits.end(), infoTag2->m_writingCredits.begin(), infoTag2->m_writingCredits.end());
      infoTag->m_director.insert(infoTag->m_director.end(), infoTag2->m_director.begin(), infoTag2->m_director.end());
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
void CPlexExtraInfoLoader::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lk(m_lock);
  if (m_jobMap.find(jobID) == m_jobMap.end())
  {
    CLog::Log(LOGERROR, "CPlexExtraInfoLoader::OnJobComplete can't find list for jobID: %d", jobID);
    return;
  }

  CFileItemList* list = m_jobMap[jobID];
  if (!list)
    return;
  m_jobMap.erase(jobID);

  CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader::OnJobComplete loaded extra info for %s, sucess: %s", list->GetPath().c_str(), success ? "YES" : "NO");

  CPlexDirectoryFetchJob* fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (!job)
      return;

  CFileItemPtr extraItem = fjob->m_items.Get(0);

  if (extraItem)
  {
    CopyProperties(list, extraItem);
    LoadExtraInfoForItem(list, extraItem);
  }

  g_directoryCache.SetDirectory(fjob->m_url.Get(), fjob->m_items, XFILE::DIR_CACHE_ALWAYS);
}


#if 0

if (boost::ends_with(m_url.GetFileName(), "/children") ||
    boost::ends_with(m_url.GetFileName(), "/allLeaves"))
{
  /* When we are asking for /children we also ask for the parent
   * path to get more information for the path we want to navigate
   * to */
  CURL augmentUrl = m_url;
  augmentUrl.SetProtocolOptions("");

  CStdString newFile = m_url.GetFileName();
  if (boost::ends_with(m_url.GetFileName(), "/children"))
    boost::replace_last(newFile, "/children", "");
  else
    boost::replace_last(newFile, "/allLeaves", "");
  augmentUrl.SetFileName(newFile);
  AddAugmentation(augmentUrl);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::AddAugmentation(const CURL &url)
{
  CSingleLock lk(m_augmentationLock);
  if (m_isCanceled)
    return;

  CLog::Log(LOGDEBUG, "CPlexDirectory::AddAugmentation adding %s", url.Get().c_str());
  int id = CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(url), this, CJob::PRIORITY_HIGH);
  m_augmentationJobs.push_back(id);
  m_isAugmented = true;
  m_augmentationEvent.Reset();
}

////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::DoAugmentation(CFileItemList &fileItems)
{
  /* Wait for the agumentation to return for 5 seconds */
  if (m_augmentationEvent.WaitMSec(5 * 1000))
  {
    CSingleLock lk(m_augmentationLock);
    if (m_isCanceled)
      return;

    BOOST_FOREACH(CFileItemList *augList, m_augmentationItems)
    {
      if (augList->Size() > 0)
      {
        CFileItemPtr augItem = augList->Get(0);
        if ((fileItems.GetPlexDirectoryType() == PLEX_DIR_TYPE_SEASON ||
             fileItems.GetPlexDirectoryType() == PLEX_DIR_TYPE_EPISODE) &&
            augItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_SHOW)
        {
          /* Augmentation of seasons works like this:
           * We load metadata/XX/children as our main url
           * then we load metadata/XX as augmentation, then we
           * augment our main CFileItem with information from the
           * first subitem from the augmentation URL.
           */

          if (augItem->HasProperty("summary"))
            fileItems.SetProperty("showplot", augItem->GetProperty("summary"));

          for (int i = 0; i < fileItems.Size(); i++)
          {
            CFileItemPtr item = fileItems.Get(i);

            std::pair<CStdString, CVariant> p;
            BOOST_FOREACH(p, augItem->m_mapProperties)
            {
              /* we only insert the properties if they are not available */
              if (item->m_mapProperties.find(p.first) == item->m_mapProperties.end())
                item->m_mapProperties[p.first] = p.second;
            }

            std::pair<CStdString, CStdString> sP;
            BOOST_FOREACH(sP, augItem->GetArt())
            {
              if (!item->HasArt(sP.first))
                item->SetArt(sP.first, sP.second);
            }

            if (augItem->HasVideoInfoTag())
            {
              CVideoInfoTag* infoTag = item->GetVideoInfoTag();
              CVideoInfoTag* infoTag2 = augItem->GetVideoInfoTag();

              infoTag->m_genre.insert(infoTag->m_genre.end(), infoTag2->m_genre.begin(), infoTag2->m_genre.end());
              infoTag->m_cast.insert(infoTag->m_cast.end(), infoTag2->m_cast.begin(), infoTag2->m_cast.end());
              infoTag->m_writingCredits.insert(infoTag->m_writingCredits.end(), infoTag2->m_writingCredits.begin(), infoTag2->m_writingCredits.end());
              infoTag->m_director.insert(infoTag->m_director.end(), infoTag2->m_director.begin(), infoTag2->m_director.end());
            }
          }
        }
        else if ((fileItems.GetPlexDirectoryType() == PLEX_DIR_TYPE_ALBUM ||
                  fileItems.GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK) &&
                 (augItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_ARTIST ||
                  augItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_ALBUM))
        {
          for (int i = 0; i < fileItems.Size(); i++)
          {
            CFileItemPtr item = fileItems.Get(i);
            if (!item)
              continue;


            std::pair<CStdString, CVariant> p;
            BOOST_FOREACH(p, augItem->m_mapProperties)
            {
              /* we only insert the properties if they are not available */
              if (item->m_mapProperties.find(p.first) == item->m_mapProperties.end())
              {
                item->m_mapProperties[p.first] = p.second;
              }
            }

            std::pair<CStdString, CStdString> sP;
            BOOST_FOREACH(sP, augItem->GetArt())
            {
              if (!item->HasArt(sP.first))
                item->SetArt(sP.first, sP.second);
            }

            if (augItem->HasMusicInfoTag())
            {
              MUSIC_INFO::CMusicInfoTag* musicInfoTag = item->GetMusicInfoTag();
              MUSIC_INFO::CMusicInfoTag* musicInfoTag2 = augItem->GetMusicInfoTag();

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
      }
    }
  }
  else
  {
    CSingleLock lk(m_augmentationLock);
    m_isCanceled = true;
    CLog::Log(LOGWARNING, "CPlexDirectory::DoAugmentation timed out");
    BOOST_FOREACH(int id, m_augmentationJobs)
      CJobManager::GetInstance().CancelJob(id);
    m_augmentationJobs.clear();
  }

  /* clean up */
  BOOST_FOREACH(CFileItemList* item, m_augmentationItems)
    delete item;
  m_augmentationItems.clear();
}


////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lk(m_augmentationLock);

  if (m_isCanceled)
    return;

  if (success)
  {
    CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
    CFileItemList* list = new CFileItemList;
    list->Copy(fjob->m_items);

    m_augmentationItems.push_back(list);

    /* Fire off some more augmentation events if needed */
    if ((list->GetPlexDirectoryType() == PLEX_DIR_TYPE_SEASON ||
         list->GetPlexDirectoryType() == PLEX_DIR_TYPE_ALBUM ||
         list->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK) &&
        list->Size() > 0 &&
        list->Get(0)->HasProperty("parentKey"))
    {
      AddAugmentation(CURL(list->Get(0)->GetProperty("parentKey").asString()));
    }
  }

  m_augmentationJobs.erase(std::remove(m_augmentationJobs.begin(), m_augmentationJobs.end(), jobID), m_augmentationJobs.end());

  if (m_augmentationJobs.size() == 0)
    m_augmentationEvent.Set();
}

#endif
