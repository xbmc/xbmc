/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryRefreshingJob.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtUtils.h"
#include "utils/Artwork.h"
#include "utils/DiscsUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/FilenameAttributes.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoDownloader.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoThumbLoader.h"
#include "video/dialogs/GUIDialogVideoManagerVersions.h"
#include "video/tags/IVideoInfoTagLoader.h"
#include "video/tags/VideoInfoTagLoaderFactory.h"
#include "video/tags/VideoTagLoaderNFO.h"
#include "video/tags/VideoTagLoaderPlugin.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace VIDEO;
using namespace ADDON;

namespace
{
// Whether a movie or music video has art next to it that CVideoInfoScannerArt::GetArtwork() only
// uses with local information.
bool HasLocalArt(const CFileItem& item, ADDON::ContentType content, bool useFolderNames)
{
  if ((content != ADDON::ContentType::MOVIES && content != ADDON::ContentType::MUSICVIDEOS) ||
      item.SkipLocalArt())
    return false;

  const int artLevel{CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL)};
  if (artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
    return false;

  const bool addAll{artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL};
  const bool exactName{artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC};
  std::vector<std::string> artTypes{CVideoThumbLoader::GetArtTypes(
      content == ADDON::ContentType::MOVIES ? MEDIA::TYPE::MOVIE : MEDIA::TYPE::MUSIC_VIDEO)};

  // the set comes from the new scrape, so look for set art whether or not the movie is in one now
  if (content == ADDON::ContentType::MOVIES)
  {
    for (const std::string& artType : CVideoThumbLoader::GetArtTypes(MEDIA::TYPE::VIDEO_COLLECTION))
      artTypes.push_back("set." + artType);
  }

  ART::Artwork art;
  bool useFolder{false};
  ART::AddLocalItemArtwork(art, artTypes, ART::GetLocalArtBaseFilename(item, useFolder), addAll,
                           exactName, useFolderNames);
  return !art.empty();
}
} // unnamed namespace

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

bool CVideoLibraryRefreshingJob::Equals(const CJob* job) const
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
  auto& localizeStrings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  if (m_item == nullptr)
    return false;

  CLog::Log(LOGDEBUG,
            "CVideoLibraryRefreshingJob: refreshing '{}' (ignore nfo: {}, refresh all: {})",
            CURL::GetRedacted(m_item->GetPath()), m_ignoreNfo, m_refreshAll);

  // determine the scraper for the item's path
  VIDEO::SScanSettings scanSettings;
  CVideoDatabase::ScraperCache scrapers;
  ScraperPtr scraper = db.GetScraperForPath(m_item->GetPath(), scanSettings, &scrapers);
  if (!scraper && m_item->GetVideoContentType() == VideoDbContentType::MOVIE_SETS)
  {
    // Deal with refreshing movie set
    // Get movies in set
    CVideoInfoTag tag{*m_item->GetVideoInfoTag()};
    const int dbId{tag.m_iDbId};
    std::string setTitle{tag.GetTitle()};
    const std::string originalSetTitle{db.GetOriginalSetById(dbId)};
    CFileItemList movies;
    CVideoInfoTag movieTag;
    bool localFound{false};
    bool onlineFound{false};
    std::string overview{};

    if (db.GetMoviesBySet(m_item->GetPath(), movies, dbId))
    {
      // Scan each movie folder for an NFO file and get online information for a movie in the set
      for (const auto& movie : movies)
      {
        // Check not plugin
        if (!URIUtils::IsPlugin(movie->GetDynPath()))
        {
          // Use local scraper
          AddonPtr addon{};
          CServiceBroker::GetAddonMgr().GetAddon("metadata.local", addon, OnlyEnabled::CHOICE_YES);
          scraper = std::dynamic_pointer_cast<CScraper>(addon);

          // See if Movie NFO
          movie->SetPath(movie->GetDynPath());
          const auto nfo{std::make_unique<CVideoTagLoaderNFO>(*movie, scraper, true)};
          if (nfo->HasInfo())
          {
            // Movie NFO found - see if it has set overview
            nfo->Load(tag, false);

            // Check set matches
            if (tag.m_set.GetTitle() == originalSetTitle)
            {
              localFound = true;
              if (tag.m_set.HasOverview())
                overview = tag.m_set.GetOverview();
            }
          }

          // Get online details (only if no movie.nfo containing set details found locally)
          if (!onlineFound && overview.empty() && !movie->GetVideoInfoTag()->GetTitle().empty())
          {
            scraper = db.GetScraperForPath(movie->GetDynPath(), scanSettings);
            CVideoInfoDownloader infoDownloader{scraper};
            MOVIELIST itemResultList;
            infoDownloader.FindMovie(movie->GetVideoInfoTag()->GetTitle(),
                                     movie->GetVideoInfoTag()->GetYear(), itemResultList);
            if (!itemResultList.empty())
            {
              CScraper::UniqueIDs uniqueIDs;
              if (infoDownloader.GetDetails(uniqueIDs, itemResultList.at(0), tag) &&
                  tag.m_set.HasTitle())
              {
                onlineFound = true;
                if (overview.empty())
                  overview = tag.m_set.GetOverview();
              }
            }
          }
        }
      }
    }

    // UpdateSetInTag will update set art using the following:
    // 1 Local files
    // 2 Set.nfo <art> tag
    // 3 Online scraper (already populated above)
    bool setUpdated{CVideoInfoScanner::UpdateSetInTag(tag)};

    // Nothing found to update with
    if (!localFound && !onlineFound && !setUpdated)
      return false;

    // tag now contains up-to-date set title
    if (tag.m_set.HasTitle())
      overview = tag.m_set.GetOverview();
    db.AddSet(tag.m_set.GetTitle(), overview, tag.m_set.GetOriginalTitle(),
              tag.m_set.GetSortTitle());

    // Update set art
    ART::Artwork movieSetArt;
    if (tag.m_set.HasArt())
      movieSetArt = tag.m_set.GetArt();
    db.SetArtForItem(dbId, MEDIA::TYPE::VIDEO_COLLECTION, movieSetArt);

    // Refresh (for video info dialog)
    m_item->ClearArt();
    db.GetSetInfo(dbId, tag);
    m_item->SetFromVideoInfoTag(tag);
    return true;
  }

  if (!scraper)
    return false;

  if (URIUtils::IsPlugin(m_item->GetPath()) &&
      !XFILE::CPluginDirectory::IsMediaLibraryScanningAllowed(TranslateContent(scraper->Content()),
                                                              m_item->GetPath()))
  {
    CLog::Log(LOGINFO,
              "CVideoLibraryRefreshingJob: Plugin '{}' does not support media library scanning and "
              "refreshing",
              CURL::GetRedacted(m_item->GetPath()));
    return false;
  }

  // copy the scraper in case we need it again
  ScraperPtr originalScraper(scraper);

  // a tv show, rather than one of its seasons or episodes, is refreshed as a movie is: removed and
  // added again
  const bool isTvShow{scraper->Content() == ADDON::ContentType::TVSHOWS &&
                      m_item->HasVideoInfoTag() &&
                      m_item->GetVideoInfoTag()->GetMediaType() == MEDIA::TYPE::TV_SHOW};

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
  bool lookUpByIds{true};
  // a title the user enters is searched for in place of one embedded in the file
  bool useEmbeddedTitle{m_searchTitle.empty()};
  do
  {
    std::unique_ptr<CVideoInfoTag> pluginTag;
    std::unique_ptr<KODI::ART::Artwork> pluginArt;
    std::string embeddedTitle;
    int embeddedYear{-1};

    if (!ignoreNfo)
    {
      // check if there's an NFO for the item
      CInfoScanner::InfoType nfoResult = CInfoScanner::InfoType::NONE;
      bool haveLoader{false};
      if (const std::unique_ptr<VIDEO::IVideoInfoTagLoader> loader{
              VIDEO::CVideoInfoTagLoaderFactory::CreateLoader(
                  *m_item, scraper, scanSettings.parent_name_root, m_forceRefresh)};
          loader)
      {
        haveLoader = true;
        std::unique_ptr<CVideoInfoTag> tag(new CVideoInfoTag());
        nfoResult = loader->Load(*tag, false);

        // keep some properties only if advancedsettings.xml says so
        const auto advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
        if (!advancedSettings->m_bVideoLibraryImportWatchedState)
          tag->ResetPlayCount();
        if (!advancedSettings->m_bVideoLibraryImportResumePoint)
          tag->SetResumePoint(CBookmark());

        if (nfoResult == CInfoScanner::InfoType::FULL && m_item->IsPlugin() &&
            scraper->ID() == "metadata.local")
        {
          // get video info and art from plugin source with metadata.local scraper
          if (scraper->Content() == ADDON::ContentType::TVSHOWS && !m_item->IsFolder() &&
              tag->m_iIdShow < 0)
            // preserve show_id for episode
            tag->m_iIdShow = m_item->GetVideoInfoTag()->m_iIdShow;
          pluginTag = std::move(tag);
          CVideoTagLoaderPlugin* nfo = dynamic_cast<CVideoTagLoaderPlugin*>(loader.get());
          if (nfo && nfo->GetArt())
            pluginArt = std::move(nfo->GetArt());
        }
        else if (nfoResult == CInfoScanner::InfoType::URL ||
                 nfoResult == CInfoScanner::InfoType::COMBINED)
          scraperUrl = loader->ScraperUrl();
        else if (nfoResult == CInfoScanner::InfoType::TITLE)
        {
          embeddedTitle = tag->GetTitle();
          embeddedYear = tag->GetYear();
        }
      }

      static constexpr std::array NFO_RESULTS{"none",     "full",      "url",  "override",
                                              "combined", "error nfo", "title"};
      const auto result{static_cast<size_t>(nfoResult)};
      CLog::Log(LOGDEBUG, "CVideoLibraryRefreshingJob: {} gave '{}' for '{}'",
                haveLoader ? "a tag loader" : "no tag loader",
                result < NFO_RESULTS.size() ? NFO_RESULTS[result] : "?",
                CURL::GetRedacted(m_item->GetPath()));

      // if there's no NFO remember it in case we have to refresh again
      if (nfoResult == CInfoScanner::InfoType::ERROR_NFO)
        ignoreNfo = true;
      // a movie's or tv show's embedded title or override nfo still needs its details looked up
      else if (nfoResult != CInfoScanner::InfoType::NONE &&
               ((scraper->Content() != ADDON::ContentType::MOVIES && !isTvShow) ||
                (nfoResult != CInfoScanner::InfoType::TITLE &&
                 nfoResult != CInfoScanner::InfoType::OVERRIDE)))
        hasDetails = true;

      // if we are performing a forced refresh ask the user to choose between using the local
      // information and a valid scraper. A local scraper has nothing to refresh from, and an nfo
      // the scraper failed on is ignored anyway. Without an nfo the answer still covers episode
      // nfos and season art, an nfo the lookup above can miss (folder items, or parent_name
      // differing from parent_name_root) and local art.
      if (needsRefresh && IsModal() && !scraper->IsNoop() && scraper->ID() != "metadata.local" &&
          nfoResult != CInfoScanner::InfoType::ERROR_NFO &&
          (nfoResult != CInfoScanner::InfoType::NONE ||
           scraper->Content() == ADDON::ContentType::TVSHOWS || VIDEO::IsBrowsableFolder(*m_item) ||
           scanSettings.parent_name != scanSettings.parent_name_root ||
           HasLocalArt(*m_item, scraper->Content(), scanSettings.parent_name)))
      {
        int heading = 20159;
        if (scraper->Content() == ADDON::ContentType::MOVIES)
          heading = 13346;
        else if (scraper->Content() == ADDON::ContentType::TVSHOWS)
          heading = m_item->IsFolder() ? 20351 : 20352;
        else if (scraper->Content() == ADDON::ContentType::MUSICVIDEOS)
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
    if (scraper->Content() == ADDON::ContentType::TVSHOWS && !m_item->IsFolder())
      hasDetails = true;

    // Without anyone to choose from what a search finds, a movie or tv show is looked up by the ids
    // it already has, as a search by its title may find another or nothing
    CScraper::UniqueIDs uniqueIDs;
    if (lookUpByIds && !hasDetails && !IsModal() && m_searchTitle.empty() && scraper->IsPython() &&
        (scraper->Content() == ADDON::ContentType::MOVIES || isTvShow) && m_item->HasVideoInfoTag())
    {
      for (const auto& [type, value] : m_item->GetVideoInfoTag()->GetUniqueIDs())
      {
        if (!value.empty())
          uniqueIDs.try_emplace(type, value);
      }
      if (!uniqueIDs.empty())
        scraper->ClearCache();
    }

    // if we don't have an url or need to refresh anyway do the web search
    if (!hasDetails && uniqueIDs.empty() && (needsRefresh || !scraperUrl.HasUrls()))
    {
      SetTitle(StringUtils::Format(localizeStrings.Get(197), scraper->Name()));
      SetText(itemTitle);
      SetProgress(0);

      // clear any cached data from the scraper
      scraper->ClearCache();

      // create the info downloader for the scraper
      CVideoInfoDownloader infoDownloader(scraper);

      // try adding by filename identifier
      if (scraper->IsPython() && CFilenameAttributes(itemTitle, nullptr).HasIdentifier())
      {
        CFileItemList items;
        items.Add(m_item);
        items.SetPath(m_item->IsFolder() ? URIUtils::GetParentPath(m_item->GetPath())
                                         : URIUtils::GetDirectory(m_item->GetPath()));
        VIDEO::CVideoInfoScanner scanner;
        if (scanner.RetrieveVideoInfo(items, scanSettings.parent_name, scraper->Content(),
                                      !ignoreNfo, nullptr, m_refreshAll, GetProgressDialog()))
        {
          return true;
        }
      }

      // try to find a matching item
      MOVIELIST itemResultList;
      const bool useTagTitle{useEmbeddedTitle && !ignoreNfo && !embeddedTitle.empty()};
      int result = infoDownloader.FindMovie(useTagTitle ? embeddedTitle : itemTitle,
                                            useTagTitle ? embeddedYear : -1, itemResultList,
                                            GetProgressDialog());

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
            selectDialog->SetHeading(scraper->Content() == ADDON::ContentType::TVSHOWS ? 20356
                                                                                       : 196);
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
              if (!CGUIKeyboardFactory::ShowAndGetInput(
                      itemTitle,
                      localizeStrings.Get(
                          scraper->Content() == ADDON::ContentType::TVSHOWS ? 20357 : 16009),
                      false))
                return false;

              // go through the whole process again
              useEmbeddedTitle = false;
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
    if (!hasDetails && uniqueIDs.empty() && !scraperUrl.HasUrls())
    {
      // a local scraper has nothing to search, so another title cannot help and asking for one
      // would only bring us back here
      if (IsModal() && scraper->ID() != "metadata.local")
      {
        // ask the user to input a title to use
        if (!CGUIKeyboardFactory::ShowAndGetInput(
                itemTitle,
                localizeStrings.Get(
                    scraper->Content() == ADDON::ContentType::TVSHOWS ? 20357 : 16009),
                false))
          return false;

        // go through the whole process again
        useEmbeddedTitle = false;
        needsRefresh = true;
        continue;
      }

      // nothing else we can do
      failure = true;
      break;
    }

    // A movie or tv show is removed before it is added again, so its details are read first. One the
    // scraper can't give them for, as when its site doesn't answer, is left as it is.
    if ((scraper->Content() == ADDON::ContentType::MOVIES || isTvShow) &&
        (scraperUrl.HasUrls() || !uniqueIDs.empty()))
    {
      // the search closed a modal progress dialog, and the download pumps the GUI through it
      SetTitle(localizeStrings.Get(198));
      SetText(itemTitle);
      SetProgress(0);

      CVideoInfoTag details;
      if (!CVideoInfoDownloader(scraper).GetDetails(uniqueIDs, scraperUrl, details,
                                                    GetProgressDialog()))
      {
        // A scraper that can't look the movie up by its ids is searched for it as before
        if (!uniqueIDs.empty())
        {
          lookUpByIds = false;
          needsRefresh = true;
          continue;
        }
        CLog::Log(LOGWARNING,
                  "CVideoLibraryRefreshingJob: no details found for '{}', so it is left as it is",
                  CURL::GetRedacted(m_item->GetPath()));
        failure = true;
        break;
      }

      // The search made when the ids lookup failed may have found another film
      if (!lookUpByIds && m_item->HasVideoInfoTag() &&
          m_item->GetVideoInfoTag()->HasConflictingUniqueID(details))
      {
        CLog::Log(LOGWARNING,
                  "CVideoLibraryRefreshingJob: the details found for '{}' are of another film by "
                  "their ids, so it is left as it is",
                  CURL::GetRedacted(m_item->GetPath()));
        failure = true;
        break;
      }
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
    std::string discFile;
    CFileItemList items;
    if (m_item->HasVideoInfoTag() && m_item->GetVideoInfoTag()->m_iDbId > 0)
    {
      // a bluray movie is refreshed from its disc, so that its playlists are chosen again. Older
      // versions stored the disc itself (the ISO or index.bdmv) rather than a playlist.
      if (const std::string & filePath{m_item->GetVideoInfoTag()->m_strFileNameAndPath};
          scraper->Content() == ADDON::ContentType::MOVIES)
      {
        if (URIUtils::IsBlurayPath(filePath))
          discFile = URIUtils::GetDiscFile(filePath);
        else if (::UTILS::DISCS::IsBlurayDiscImage(filePath) || URIUtils::IsBDFile(filePath))
          discFile = filePath;

        // A disc holding another movie too is found as that movie once this one is removed, so
        // this one is refreshed from its own file alone
        if (!discFile.empty())
        {
          const int playlistPathId{db.GetPathId(URIUtils::GetBlurayPlaylistPath(discFile))};
          if (playlistPathId >= 0 &&
              db.GetSingleValueInt(db.PrepareSQL(
                  "SELECT COUNT(1) FROM movie JOIN files ON files.idFile = movie.idFile "
                  "WHERE files.idPath = %i AND movie.idMovie <> %i",
                  playlistPathId, m_item->GetVideoInfoTag()->m_iDbId)) > 0)
            discFile.clear();
        }
      }

      // for a tvshow we need to handle all paths of it
      std::vector<std::string> tvshowPaths;
      if (m_item->GetVideoInfoTag()->GetMediaType() == MEDIA::TYPE::TV_SHOW && m_refreshAll &&
          db.GetPathsLinkedToTvShow(m_item->GetVideoInfoTag()->m_iDbId, tvshowPaths))
      {
        for (const auto& tvshowPath : tvshowPaths)
        {
          CFileItemPtr tvshowItem(new CFileItem(*m_item->GetVideoInfoTag()));
          tvshowItem->SetPath(tvshowPath);
          items.Add(tvshowItem);
        }
      }
      else if (!discFile.empty())
        items.Add(std::make_shared<CFileItem>(discFile, false));
      // otherwise just add a copy of the item
      else
        items.Add(std::make_shared<CFileItem>(*m_item->GetVideoInfoTag()));

      // update the path to the real path (instead of a videodb:// one)
      path = discFile.empty() ? m_item->GetVideoInfoTag()->m_strPath : discFile;
    }
    else
      items.Add(std::make_shared<CFileItem>(*m_item));

    // an item created from a video info tag is given the default icon, which is not art to save
    for (const auto& item : items)
      item->ClearArt();

    // set the proper path of the list of items to lookup
    items.SetPath(VIDEO::IsBrowsableFolder(*m_item) ? URIUtils::GetParentPath(path)
                                                    : URIUtils::GetDirectory(path));

    int headingLabel = 198;
    if (scraper->Content() == ADDON::ContentType::TVSHOWS)
    {
      if (m_item->IsFolder())
        headingLabel = 20353;
      else
        headingLabel = 20361;
    }
    else if (scraper->Content() == ADDON::ContentType::MUSICVIDEOS)
      headingLabel = 20394;

    // prepare the progress dialog for downloading all the necessary information
    SetTitle(localizeStrings.Get(headingLabel));
    SetText(itemTitle);
    SetProgress(0);

    const int origDbId{m_item->GetVideoInfoTag()->m_iDbId};
    // Read from the database, as a scan may have added versions or extras to the movie since the
    // item was read, such as while its information dialog was open. The movie's own file is one
    // of its assets.
    const bool hasAdditionalAssets{
        m_item->HasVideoVersions() || m_item->HasVideoExtras() ||
        (origDbId > 0 && scraper->Content() == ADDON::ContentType::MOVIES &&
         db.GetSingleValueInt(db.PrepareSQL(
             "SELECT COUNT(1) FROM videoversion WHERE idMedia = %i AND media_type = '%s'", origDbId,
             MEDIA::NameOf(MEDIA::TYPE::MOVIE).c_str())) > 1)};

    // the disc's watched state, carried over if a single playlist is found on it again
    CVideoDatabase::FileRecord discFileState;
    CBookmark discResumePoint;
    VECBOOKMARKS discBookmarks;
    if (!discFile.empty())
    {
      const CVideoInfoTag& tag{*m_item->GetVideoInfoTag()};
      discFileState = {.m_idFile = tag.m_iFileId,
                       .m_playCount = tag.GetPlayCount(),
                       .m_lastPlayed = tag.m_lastPlayed,
                       .m_dateAdded = tag.m_dateAdded};
      db.GetResumeBookMark(tag.m_strFileNameAndPath, discResumePoint);
      db.GetBookMarksForFile(tag.m_strFileNameAndPath, discBookmarks);
    }

    // A movie or episode is added again for the same files, so the hashes of its folders are kept
    // and the next scan doesn't look at them again. They are cleared should adding it again fail.
    std::string mediaPath;

    // remove any existing data for the item we're going to refresh
    if (origDbId > 0)
    {
      if (scraper->Content() == ADDON::ContentType::MOVIES)
      {
        mediaPath = db.GetSingleValue(
            db.PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath = path.idPath "
                          "JOIN movie ON movie.idFile = files.idFile WHERE movie.idMovie = %i",
                          origDbId));

        // the disc's other playlists are chosen again along with it
        if (!discFile.empty())
        {
          CFileItemList versions;
          db.GetVideoVersions(VideoDbContentType::MOVIES, origDbId, versions,
                              VideoAssetType::VERSION);
          for (const auto& version : versions)
          {
            if (URIUtils::GetDiscFile(version->GetDynPath()) == discFile)
              db.DeleteVideoAsset(version->GetVideoInfoTag()->m_iDbId,
                                  DeleteMovieHashAction::HASH_PRESERVE);
          }
        }
        db.DeleteMovie(origDbId, DeleteMovieCascadeAction::DEFAULT_VERSION,
                       DeleteMovieHashAction::HASH_PRESERVE);
      }
      else if (scraper->Content() == ADDON::ContentType::MUSICVIDEOS)
        db.DeleteMusicVideo(origDbId);
      else if (scraper->Content() == ADDON::ContentType::TVSHOWS)
      {
        if (!m_item->IsFolder())
        {
          mediaPath = db.GetSingleValue(db.PrepareSQL(
              "SELECT strPath FROM path JOIN files ON files.idPath = path.idPath "
              "JOIN episode ON episode.idFile = files.idFile WHERE episode.idEpisode = %i",
              origDbId));
          db.DeleteEpisode(origDbId, false, DeleteMovieHashAction::HASH_PRESERVE);
        }
        else if (m_item->GetVideoInfoTag()->GetMediaType() == MEDIA::TYPE::SEASON)
          db.DeleteSeason(origDbId);
        else if (m_refreshAll)
          db.DeleteTvShow(origDbId);
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

    // finally download the information for the item. A movie's extras are added once its old
    // versions and extras are its own again, so that those already there are seen.
    VIDEO::CVideoInfoScanner scanner;
    scanner.DeferMovieExtras();
    scanner.UseScrapers(scrapers);
    scanner.LookUpBy(uniqueIDs);
    if (!scanner.RetrieveVideoInfo(items, scanSettings.parent_name,
                                   scraper->Content(), !ignoreNfo,
                                   scraperUrl.HasUrls() ? &scraperUrl : nullptr,
                                   m_refreshAll, GetProgressDialog()))
    {
      // something went wrong
      MarkFinished();

      if (!mediaPath.empty())
        db.InvalidatePathHash(mediaPath);

      // The movie has been removed. Remove its other versions and extras too, as removing it from
      // the library would, rather than leave them attached to a movie that no longer exists.
      if (hasAdditionalAssets && scraper->Content() == ADDON::ContentType::MOVIES)
        db.DeleteMovie(origDbId, DeleteMovieCascadeAction::ALL_ASSETS);

      // check if the user cancelled
      if (!IsCancelled() && IsModal())
        HELPERS::ShowOKDialogText(CVariant{195}, CVariant{itemTitle});

      return false;
    }

    // retrieve the updated information from the database
    if (scraper->Content() == ADDON::ContentType::MOVIES)
    {
      // the disc's main playlist may differ from the one refreshed, so the movie is found by the
      // disc's playlists, as a file left by an older version's disc entry shadows the disc itself
      const int playlistPathId{
          discFile.empty() ? -1 : db.GetPathId(URIUtils::GetBlurayPlaylistPath(discFile))};
      const int idMovie{playlistPathId < 0
                            ? 0
                            : db.GetSingleValueInt(db.PrepareSQL(
                                  "SELECT idMovie FROM movie JOIN files ON files.idFile = "
                                  "movie.idFile WHERE files.idPath = %i",
                                  playlistPathId))};
      if (idMovie > 0 && db.GetMovieInfo({}, *m_item->GetVideoInfoTag(), idMovie))
      {
        const CVideoInfoTag& tag{*m_item->GetVideoInfoTag()};

        // it is only known which playlist was watched when the disc has just the one
        if (discFileState.m_idFile > 0 && tag.m_iFileId != discFileState.m_idFile &&
            db.GetSingleValueInt(db.PrepareSQL(
                "SELECT COUNT(1) FROM videoversion "
                "JOIN files ON files.idFile = videoversion.idFile "
                "WHERE files.idPath = %i AND videoversion.media_type = '%s' AND "
                "videoversion.itemType = %i",
                playlistPathId, MEDIA::NameOf(MEDIA::TYPE::MOVIE).c_str(), static_cast<int>(VideoAssetType::VERSION))) == 1)
        {
          // UNKNOWN, as with MOVIES SetFileForMovie() keeps the old file and its settings when the
          // new one is already a version of the movie, which the playlist is. This moves the file's
          // state and settings and removes the old file, which takes its bookmarks with it, so they
          // are added again before the move is committed.
          db.BeginTransaction();
          const std::string path{tag.m_strFileNameAndPath};
          if (db.SetFileForMedia(path, VideoDbContentType::UNKNOWN, idMovie, discFileState) > 0 &&
              (!discResumePoint.IsSet() ||
               db.AddBookMarkToFile(path, discResumePoint, CBookmark::RESUME)) &&
              std::ranges::all_of(discBookmarks, [&db, &path](const CBookmark& bookmark)
                                  { return db.AddBookMarkToFile(path, bookmark); }))
          {
            db.CommitTransaction();
            db.GetMovieInfo({}, *m_item->GetVideoInfoTag(), idMovie);
          }
          else
            db.RollbackTransaction();
        }
        m_item->SetPath(m_item->GetVideoInfoTag()->m_strFileNameAndPath);
      }
      else
        db.GetMovieInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    }
    else if (scraper->Content() == ADDON::ContentType::MUSICVIDEOS)
      db.GetMusicVideoInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    else if (scraper->Content() == ADDON::ContentType::TVSHOWS)
    {
      // update tvshow/season info to get updated episode numbers
      if (m_item->IsFolder())
      {
        // Note: don't use any database ids (m_iDbId, m_idSeason, m_IdShow) of m_item's video
        // info tag here. The db information might have been deleted and recreated afterwards,
        // invalidating the old db ids and m_item is not (yet) updated at this point.
        bool hasInfo = false;
        const CVideoInfoTag* videoTag = m_item->GetVideoInfoTag();
        if (videoTag && videoTag->GetMediaType() == MEDIA::TYPE::SEASON && videoTag->m_iSeason != -1)
          hasInfo = db.GetSeasonInfo(m_item->GetPath(), videoTag->m_iSeason,
                                     *m_item->GetVideoInfoTag(), m_item.get());
        if (!hasInfo)
          db.GetTvShowInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
      }
      else
        db.GetEpisodeInfo(m_item->GetPath(), *m_item->GetVideoInfoTag());
    }

    if (hasAdditionalAssets)
    {
      // the versions kept from before the refresh are not scanned, so their stream details are
      // read again here
      CFileItemList keptVersions;
      if (scraper->Content() == ADDON::ContentType::MOVIES)
        db.GetVideoVersions(VideoDbContentType::MOVIES, origDbId, keptVersions,
                            VideoAssetType::VERSION);

      const auto videoTag{m_item->GetVideoInfoTag()};
      db.UpdateAssetsOwner(videoTag->GetMediaType(), origDbId, videoTag->m_iDbId);

      // The versions kept are on other discs than the one refreshed, so the movie the disc was added
      // as again isn't named by it
      if (!keptVersions.IsEmpty() && videoTag->m_iDbId > 0)
      {
        CGUIDialogVideoManagerVersions::RemovePartNumberFromTitle(videoTag->m_iDbId,
                                                                  VideoDbContentType::MOVIES, db);
        db.GetMovieInfo({}, *videoTag, videoTag->m_iDbId);
      }

      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
        keptVersions.Clear();
      for (const auto& version : keptVersions)
      {
        CFileItem versionItem{version->GetDynPath(), false};
        if (URIUtils::IsBlurayPath(versionItem.GetPath())
                ? XFILE::CDiscDirectoryHelper::ReadResolvedPlaylist(versionItem)
                : CDVDFileInfo::GetFileStreamDetails(&versionItem))
        {
          db.SetStreamDetailsForFileId(versionItem.GetVideoInfoTag()->m_streamDetails,
                                       version->GetVideoInfoTag()->m_iDbId);
          CLog::LogF(LOGDEBUG, "Extracted filestream details from video version {}",
                     CURL::GetRedacted(versionItem.GetPath()));
        }
        else
          CLog::LogF(LOGDEBUG, "No filestream details extracted from video version {}",
                     CURL::GetRedacted(versionItem.GetPath()));
      }
    }

    // as the movie's folders are not listed, its discs and extras folders are looked at here
    if (scraper->Content() == ADDON::ContentType::MOVIES && m_item->GetVideoInfoTag()->m_iDbId > 0)
      scanner.AddMovieExtras(m_item->GetVideoInfoTag()->m_iDbId, scanSettings.parent_name);

    // we're finally done
    MarkFinished();
    break;
  } while (needsRefresh);

  if (failure && IsModal())
    HELPERS::ShowOKDialogText(CVariant{195}, CVariant{itemTitle});

  return true;
}
