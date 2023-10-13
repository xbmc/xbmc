/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryRefreshingJob.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "addons/Scraper.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaType.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoDownloader.h"
#include "video/VideoInfoScanner.h"
#include "video/tags/IVideoInfoTagLoader.h"
#include "video/tags/VideoInfoTagLoaderFactory.h"
#include "video/tags/VideoTagLoaderPlugin.h"

#include <memory>
#include <utility>

using namespace KODI::MESSAGING;
using namespace VIDEO;

CVideoLibraryRefreshingJob::CVideoLibraryRefreshingJob(std::shared_ptr<CFileItem> item,
                                                       bool forceRefresh,
                                                       bool refreshAll,
                                                       bool ignoreNfo /* = false */,
                                                       const std::string& searchTitle /* = "" */)
  : CVideoLibraryProgressJob(nullptr),
    m_item(std::move(item)),
    m_forceRefresh(forceRefresh),
    m_refreshAll(refreshAll),
    m_ignoreNfo(ignoreNfo),
    m_searchTitle(searchTitle)
{ }

CVideoLibraryRefreshingJob::~CVideoLibraryRefreshingJob() = default;

bool CVideoLibraryRefreshingJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryRefreshingJob* refreshingJob = dynamic_cast<const CVideoLibraryRefreshingJob*>(job);
  if (refreshingJob == nullptr)
    return false;

  return m_item->GetPath() == refreshingJob->m_item->GetPath();
}

bool CVideoLibraryRefreshingJob::Work(CVideoDatabase &db)
{
  if (m_item == nullptr)
    return false;

  // determine the scraper for the item's path
  VIDEO::SScanSettings scanSettings;
  ADDON::ScraperPtr scraper = db.GetScraperForPath(m_item->GetPath(), scanSettings);
  if (scraper == nullptr)
    return false;

  if (URIUtils::IsPlugin(m_item->GetPath()) && !XFILE::CPluginDirectory::IsMediaLibraryScanningAllowed(ADDON::TranslateContent(scraper->Content()), m_item->GetPath()))
  {
    CLog::Log(LOGINFO,
              "CVideoLibraryRefreshingJob: Plugin '{}' does not support media library scanning and "
              "refreshing",
              CURL::GetRedacted(m_item->GetPath()));
    return false;
  }

  // copy the scraper in case we need it again
  ADDON::ScraperPtr originalScraper(scraper);

  // get the item's correct title
  std::string itemTitle = m_searchTitle;
  if (itemTitle.empty())
    itemTitle = m_item->GetMovieName(scanSettings.parent_name);

  CScraperUrl scraperUrl;
  bool needsRefresh = m_forceRefresh;
  bool hasDetails = false;
  bool ignoreNfo = m_ignoreNfo;

  // run this in a loop in case we need to refresh again
  bool failure = false;
  do
  {
    std::unique_ptr<CVideoInfoTag> pluginTag;
    std::unique_ptr<CGUIListItem::ArtMap> pluginArt;

    if (!ignoreNfo)
    {
      std::unique_ptr<IVideoInfoTagLoader> loader;
      loader.reset(CVideoInfoTagLoaderFactory::CreateLoader(*m_item, scraper,
                                                            scanSettings.parent_name_root, m_forceRefresh));
      // check if there's an NFO for the item
      CInfoScanner::INFO_TYPE nfoResult = CInfoScanner::NO_NFO;
      if (loader)
      {
        std::unique_ptr<CVideoInfoTag> tag(new CVideoInfoTag());
        nfoResult = loader->Load(*tag, false);
        if (nfoResult == CInfoScanner::FULL_NFO && m_item->IsPlugin() && scraper->ID() == "metadata.local")
        {
          // get video info and art from plugin source with metadata.local scraper
          if (scraper->Content() == CONTENT_TVSHOWS && !m_item->m_bIsFolder && tag->m_iIdShow < 0)
            // preserve show_id for episode
            tag->m_iIdShow = m_item->GetVideoInfoTag()->m_iIdShow;
          pluginTag = std::move(tag);
          CVideoTagLoaderPlugin* nfo = dynamic_cast<CVideoTagLoaderPlugin*>(loader.get());
          if (nfo && nfo->GetArt())
            pluginArt = std::move(nfo->GetArt());
        }
        else if (nfoResult == CInfoScanner::URL_NFO)
          scraperUrl = loader->ScraperUrl();
      }

      // if there's no NFO remember it in case we have to refresh again
      if (nfoResult == CInfoScanner::ERROR_NFO)
        ignoreNfo = true;
      else if (nfoResult != CInfoScanner::NO_NFO)
        hasDetails = true;

      // if we are performing a forced refresh ask the user to choose between using a valid NFO and a valid scraper
      if (needsRefresh && IsModal() && !scraper->IsNoop()
          && nfoResult != CInfoScanner::ERROR_NFO)
      {
        int heading = 20159;
        if (scraper->Content() == CONTENT_MOVIES)
          heading = 13346;
        else if (scraper->Content() == CONTENT_TVSHOWS)
          heading = m_item->m_bIsFolder ? 20351 : 20352;
        else if (scraper->Content() == CONTENT_MUSICVIDEOS)
          heading = 20393;
        if (CGUIDialogYesNo::ShowAndGetInput(heading, 20446))
        {
          hasDetails = false;
          ignoreNfo = true;
          scraperUrl.Clear();
          scraper = originalScraper;
        }
      }
    }

    // no need to re-fetch the episode guide for episodes
    if (scraper->Content() == CONTENT_TVSHOWS && !m_item->m_bIsFolder)
      hasDetails = true;

    // if we don't have an url or need to refresh anyway do the web search
    if (!hasDetails && (needsRefresh || !scraperUrl.HasUrls()))
    {
      SetTitle(StringUtils::Format(g_localizeStrings.Get(197), scraper->Name()));
      SetText(itemTitle);
      SetProgress(0);

      // clear any cached data from the scraper
      scraper->ClearCache();

      // create the info downloader for the scraper
      CVideoInfoDownloader infoDownloader(scraper);

      // try to find a matching item
      MOVIELIST itemResultList;
      int result = infoDownloader.FindMovie(itemTitle, -1, itemResultList, GetProgressDialog());

      // close the progress dialog
      MarkFinished();

      if (result > 0)
      {
        // there are multiple matches for the item
        if (!itemResultList.empty())
        {
          // choose the first match
          if (!IsModal())
            scraperUrl = itemResultList.at(0);
          else
          {
            // ask the user what to do
            CGUIDialogSelect* selectDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
            selectDialog->Reset();
            selectDialog->SetHeading(scraper->Content() == CONTENT_TVSHOWS ? 20356 : 196);
            for (const auto& itemResult : itemResultList)
              selectDialog->Add(itemResult.GetTitle());
            selectDialog->EnableButton(true, 413); // "Manual"
            selectDialog->Open();

            // check if the user has chosen one of the results
            int selectedItem = selectDialog->GetSelectedItem();
            if (selectedItem >= 0)
              scraperUrl = itemResultList.at(selectedItem);
            // the user hasn't chosen one of the results and but has chosen to manually enter a title to use
            else if (selectDialog->IsButtonPressed())
            {
              // ask the user to input a title to use
              if (!CGUIKeyboardFactory::ShowAndGetInput(itemTitle, g_localizeStrings.Get(scraper->Content() == CONTENT_TVSHOWS ? 20357 : 16009), false))
                return false;

              // go through the whole process again
              needsRefresh = true;
              continue;
            }
            // nothing else we can do
            else
              return false;
          }

          CLog::Log(LOGDEBUG, "CVideoLibraryRefreshingJob: user selected item '{}' with URL '{}'",
                    scraperUrl.GetTitle(), scraperUrl.GetFirstThumbUrl());
        }
      }
      else if (result < 0 || !VIDEO::CVideoInfoScanner::DownloadFailed(GetProgressDialog()))
      {
        failure = true;
        break;
      }
    }

    // if the URL is still empty, check whether or not we're allowed
    // to prompt and ask the user to input a new search title
    if (!hasDetails && !scraperUrl.HasUrls())
    {
      if (IsModal())
      {
        // ask the user to input a title to use
        if (!CGUIKeyboardFactory::ShowAndGetInput(itemTitle, g_localizeStrings.Get(scraper->Content() == CONTENT_TVSHOWS ? 20357 : 16009), false))
          return false;

        // go through the whole process again
        needsRefresh = true;
        continue;
      }

      // nothing else we can do
      failure = true;
      break;
    }

    // before we start downloading all the necessary information cleanup any existing artwork and hashes
    CTextureDatabase textureDb;
    if (textureDb.Open())
    {
      for (const auto& artwork : m_item->GetArt())
        textureDb.InvalidateCachedTexture(artwork.second);

      textureDb.Close();
    }
    m_item->ClearArt();

    // put together the list of items to refresh
    std::string path = m_item->GetPath();
    CFileItemList items;
    if (m_item->HasVideoInfoTag() && m_item->GetVideoInfoTag()->m_iDbId > 0)
    {
      // for a tvshow we need to handle all paths of it
      std::vector<std::string> tvshowPaths;
      if (CMediaTypes::IsMediaType(m_item->GetVideoInfoTag()->m_type, MediaTypeTvShow) && m_refreshAll &&
          db.GetPathsLinkedToTvShow(m_item->GetVideoInfoTag()->m_iDbId, tvshowPaths))
      {
        for (const auto& tvshowPath : tvshowPaths)
        {
          CFileItemPtr tvshowItem(new CFileItem(*m_item->GetVideoInfoTag()));
          tvshowItem->SetPath(tvshowPath);
          items.Add(tvshowItem);
        }
      }
      // otherwise just add a copy of the item
      else
        items.Add(std::make_shared<CFileItem>(*m_item->GetVideoInfoTag()));

      // update the path to the real path (instead of a videodb:// one)
      path = m_item->GetVideoInfoTag()->m_strPath;
    }
    else
      items.Add(std::make_shared<CFileItem>(*m_item));

    // set the proper path of the list of items to lookup
    items.SetPath(m_item->m_bIsFolder ? URIUtils::GetParentPath(path) : URIUtils::GetDirectory(path));

    int headingLabel = 198;
    if (scraper->Content() == CONTENT_TVSHOWS)
    {
      if (m_item->m_bIsFolder)
        headingLabel = 20353;
      else
        headingLabel = 20361;
    }
    else if (scraper->Content() == CONTENT_MUSICVIDEOS)
      headingLabel = 20394;

    // prepare the progress dialog for downloading all the necessary information
    SetTitle(g_localizeStrings.Get(headingLabel));
    SetText(itemTitle);
    SetProgress(0);

    // remove any existing data for the item we're going to refresh
    if (m_item->GetVideoInfoTag()->m_iDbId > 0)
    {
      int dbId = m_item->GetVideoInfoTag()->m_iDbId;
      if (scraper->Content() == CONTENT_MOVIES)
        db.DeleteMovie(dbId);
      else if (scraper->Content() == CONTENT_MUSICVIDEOS)
        db.DeleteMusicVideo(dbId);
      else if (scraper->Content() == CONTENT_TVSHOWS)
      {
        if (!m_item->m_bIsFolder)
          db.DeleteEpisode(dbId);
        else if (m_item->GetVideoInfoTag()->m_type == MediaTypeSeason)
          db.DeleteSeason(dbId);
        else if (m_refreshAll)
          db.DeleteTvShow(dbId);
        else
          db.DeleteDetailsForTvShow(dbId);
      }
    }

    if (pluginTag || pluginArt)
    {
      // set video info and art from plugin source with metadata.local scraper to items
      for (auto &i: items)
      {
        if (pluginTag)
          *i->GetVideoInfoTag() = *pluginTag;
        if (pluginArt)
          i->SetArt(*pluginArt);
      }
    }

    // finally download the information for the item
    CVideoInfoScanner scanner;
    if (!scanner.RetrieveVideoInfo(items, scanSettings.parent_name,
                                   scraper->Content(), !ignoreNfo,
                                   scraperUrl.HasUrls() ? &scraperUrl : nullptr,
                                   m_refreshAll, GetProgressDialog()))
    {
      // something went wrong
      MarkFinished();

      // check if the user cancelled
      if (!IsCancelled() && IsModal())
        HELPERS::ShowOKDialogText(CVariant{195}, CVariant{itemTitle});

      return false;
    }

    // retrieve the updated information from the database
    if (scraper->Content() == CONTENT_MOVIES)
      db.GetMovieInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    else if (scraper->Content() == CONTENT_MUSICVIDEOS)
      db.GetMusicVideoInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    else if (scraper->Content() == CONTENT_TVSHOWS)
    {
      // update tvshow/season info to get updated episode numbers
      if (m_item->m_bIsFolder)
      {
        // Note: don't use any database ids (m_iDbId, m_idSeason, m_IdShow) of m_item's video
        // info tag here. The db information might have been deleted and recreated afterwards,
        // invalidating the old db ids and m_item is not (yet) updated at this point.
        bool hasInfo = false;
        const CVideoInfoTag* videoTag = m_item->GetVideoInfoTag();
        if (videoTag && videoTag->m_type == MediaTypeSeason && videoTag->m_iSeason != -1)
          hasInfo = db.GetSeasonInfo(m_item->GetPath(), videoTag->m_iSeason,
                                     *m_item->GetVideoInfoTag(), m_item.get());
        if (!hasInfo)
          db.GetTvShowInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
      }
      else
        db.GetEpisodeInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    }

    // we're finally done
    MarkFinished();
    break;
  } while (needsRefresh);

  if (failure && IsModal())
    HELPERS::ShowOKDialogText(CVariant{195}, CVariant{itemTitle});

  return true;
}
