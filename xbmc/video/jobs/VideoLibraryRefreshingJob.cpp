/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoLibraryRefreshingJob.h"
#include "NfoFile.h"
#include "TextureDatabase.h"
#include "addons/Scraper.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoDownloader.h"
#include "video/VideoInfoScanner.h"

CVideoLibraryRefreshingJob::CVideoLibraryRefreshingJob(CFileItemPtr item, bool forceRefresh, bool refreshAll, bool ignoreNfo /* = false */, const std::string& searchTitle /* = "" */)
  : CVideoLibraryProgressJob(nullptr),
    m_item(item),
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

  // copy the scraper in case we need it again
  ADDON::ScraperPtr originalScraper(scraper);

  // get the item's correct title
  std::string itemTitle = m_searchTitle;
  if (itemTitle.empty())
    itemTitle = m_item->GetMovieName(scanSettings.parent_name);

  CScraperUrl scraperUrl;
  VIDEO::CVideoInfoScanner scanner;
  bool needsRefresh = m_forceRefresh;
  bool hasDetails = false;
  bool ignoreNfo = m_ignoreNfo;

  // run this in a loop in case we need to refresh again
  bool failure = false;
  do
  {
    if (!ignoreNfo)
    {
      // check if there's an NFO for the item
      CNfoFile::NFOResult nfoResult = scanner.CheckForNFOFile(m_item.get(), scanSettings.parent_name_root, scraper, scraperUrl);
      // if there's no NFO remember it in case we have to refresh again
      if (nfoResult == CNfoFile::ERROR_NFO)
        ignoreNfo = true;
      else if (nfoResult != CNfoFile::NO_NFO)
        hasDetails = true;


      // if we are performing a forced refresh ask the user to choose between using a valid NFO and a valid scraper
      if (needsRefresh && IsModal() && !scraper->IsNoop()
          && nfoResult != CNfoFile::ERROR_NFO)
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
    if (!hasDetails && (needsRefresh || scraperUrl.m_url.empty()))
    {
      SetTitle(StringUtils::Format(g_localizeStrings.Get(197).c_str(), scraper->Name().c_str()));
      SetText(itemTitle);
      SetProgress(0);

      // clear any cached data from the scraper
      scraper->ClearCache();

      // create the info downloader for the scraper
      CVideoInfoDownloader infoDownloader(scraper);

      // try to find a matching item
      MOVIELIST itemResultList;
      int result = infoDownloader.FindMovie(itemTitle, itemResultList, GetProgressDialog());

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
            CGUIDialogSelect* selectDialog = g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
            selectDialog->Reset();
            selectDialog->SetHeading(scraper->Content() == CONTENT_TVSHOWS ? 20356 : 196);
            for (const auto& itemResult : itemResultList)
              selectDialog->Add(itemResult.strTitle);
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

          CLog::Log(LOGDEBUG, "CVideoLibraryRefreshingJob: user selected item '%s' with URL '%s'", scraperUrl.strTitle.c_str(), scraperUrl.m_url.at(0).m_url.c_str());
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
    if (!hasDetails && scraperUrl.m_url.empty())
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
        items.Add(CFileItemPtr(new CFileItem(*m_item->GetVideoInfoTag())));

      // update the path to the real path (instead of a videodb:// one)
      path = m_item->GetVideoInfoTag()->m_strPath;
    }
    else
      items.Add(CFileItemPtr(new CFileItem(*m_item)));

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
    SetText(scraperUrl.strTitle);
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
        else if (m_refreshAll)
          db.DeleteTvShow(dbId);
        else
          db.DeleteDetailsForTvShow(dbId);
      }
    }

    // finally download the information for the item
    if (!scanner.RetrieveVideoInfo(items, scanSettings.parent_name,
                                   scraper->Content(), !ignoreNfo,
                                   scraperUrl.m_url.empty() ? NULL : &scraperUrl,
                                   m_refreshAll, GetProgressDialog()))
    {
      // something went wrong
      MarkFinished();

      // check if the user cancelled
      if (!IsCancelled() && IsModal())
        CGUIDialogOK::ShowAndGetInput(195, itemTitle);

      return false;
    }

    // retrieve the updated information from the database
    if (scraper->Content() == CONTENT_MOVIES)
      db.GetMovieInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    else if (scraper->Content() == CONTENT_MUSICVIDEOS)
      db.GetMusicVideoInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    else if (scraper->Content() == CONTENT_TVSHOWS)
    {
      // update tvshow info to get updated episode numbers
      if (m_item->m_bIsFolder)
        db.GetTvShowInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
      else
        db.GetEpisodeInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    }

    // we're finally done
    MarkFinished();
    break;
  } while (needsRefresh);

  if (failure && IsModal())
    CGUIDialogOK::ShowAndGetInput(195, itemTitle);

  return true;
}
