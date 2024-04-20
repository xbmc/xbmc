/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVideoNav.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicDatabase.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDbUrl.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoLibraryQueue.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "view/GUIViewState.h"

#include <utility>

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;
using namespace KODI;
using namespace KODI::MESSAGING;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNSEARCH          8
#define CONTROL_LABELFILES        12

#define CONTROL_BTN_FILTER        19
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14
#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_LABELEMPTY        18

#define CONTROL_UPDATE_LIBRARY    20

CGUIWindowVideoNav::CGUIWindowVideoNav(void)
    : CGUIWindowVideoBase(WINDOW_VIDEO_NAV, "MyVideoNav.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoNav::~CGUIWindowVideoNav(void) = default;

bool CGUIWindowVideoNav::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->IsParentFolder())
      return false;

    if (pItem && pItem->HasVideoInfoTag())
    {
      CVideoLibraryQueue::GetInstance().MarkAsWatched(pItem, pItem->GetVideoInfoTag()->GetPlayCount() == 0);
      return true;
    }
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("");
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      /* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);

      SetProperty("flattened", CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN));
      if (message.GetNumStringParams() && StringUtils::EqualsNoCase(message.GetStringParam(0), "Files") &&
          CMediaSourceSettings::GetInstance().GetSources("video")->empty())
      {
        message.SetStringParam("");
      }

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;


      if (message.GetStringParam(0) != "")
      {
        CURL url(message.GetStringParam(0));

        int i = 0;
        for (; i < m_vecItems->Size(); i++)
        {
          CFileItemPtr pItem = m_vecItems->Get(i);

          // skip ".."
          if (pItem->IsParentFolder())
            continue;

          if (URIUtils::PathEquals(pItem->GetPath(), message.GetStringParam(0), true, true))
          {
            m_viewControl.SetSelectedItem(i);
            i = -1;
            if (url.GetOption("showinfo") == "true")
            {
              OnItemInfo(*pItem);
            }
            break;
          }
        }
        if (i >= m_vecItems->Size())
        {
          SelectFirstUnwatched();

          if (url.GetOption("showinfo") == "true")
          {
            // We are here if the item is filtered out in the nav window
            const std::string& path = message.GetStringParam(0);
            CFileItem item(path, URIUtils::HasSlashAtEnd(path));
            if (VIDEO::IsVideoDb(item))
            {
              *(item.GetVideoInfoTag()) = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item.GetPath()));
              if (!item.GetVideoInfoTag()->IsEmpty())
              {
                item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
                OnItemInfo(item);
              }
            }
          }
        }
      }
      else
      {
        // This needs to be done again, because the initialization of CGUIWindow overwrites it with default values
        // Mostly affects cases where GUIWindowVideoNav is constructed and we're already in a show, e.g. entering from the homescreen
        SelectFirstUnwatched();
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNPARTYMODE)
      {
        if (g_partyModeManager.IsEnabled())
          g_partyModeManager.Disable();
        else
        {
          if (!g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO))
          {
            SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE,false);
            return false;
          }

          // Playlist directory is the root of the playlist window
          if (m_guiState)
            m_guiState->SetPlaylistDirectory("playlistvideo://");

          return true;
        }
        UpdateButtons();
      }

      if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        CMediaSettings::GetInstance().CycleWatchedMode(m_vecItems->GetContent());
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        OnFilterItems(GetProperty("filter").asString());
        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (CMediaSettings::GetInstance().GetWatchedMode(m_vecItems->GetContent()) == WatchedModeAll)
          CMediaSettings::GetInstance().SetWatchedMode(m_vecItems->GetContent(), WatchedModeUnwatched);
        else
          CMediaSettings::GetInstance().SetWatchedMode(m_vecItems->GetContent(), WatchedModeAll);
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        OnFilterItems(GetProperty("filter").asString());
        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_UPDATE_LIBRARY)
      {
        if (!CVideoLibraryQueue::GetInstance().IsScanningLibrary())
          OnScan("");
        else
          CVideoLibraryQueue::GetInstance().StopLibraryScanning();
        return true;
      }
    }
    break;
    // update the display
    case GUI_MSG_REFRESH_THUMBS:
      Refresh();
      break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

SelectFirstUnwatchedItem CGUIWindowVideoNav::GetSettingSelectFirstUnwatchedItem()
{
  if (VIDEO::IsVideoDb(*m_vecItems))
  {
    NODE_TYPE nodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_vecItems->GetPath());

    if (nodeType == NODE_TYPE_SEASONS || nodeType == NODE_TYPE_EPISODES)
    {
      int iValue = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM);
      if (iValue >= SelectFirstUnwatchedItem::NEVER && iValue <= SelectFirstUnwatchedItem::ALWAYS)
        return (SelectFirstUnwatchedItem)iValue;
    }
  }

  return SelectFirstUnwatchedItem::NEVER;
}

IncludeAllSeasonsAndSpecials CGUIWindowVideoNav::GetSettingIncludeAllSeasonsAndSpecials()
{
  int iValue = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS);
  if (iValue >= IncludeAllSeasonsAndSpecials::NEITHER && iValue <= IncludeAllSeasonsAndSpecials::SPECIALS)
    return (IncludeAllSeasonsAndSpecials)iValue;

  return IncludeAllSeasonsAndSpecials::NEITHER;
}

int CGUIWindowVideoNav::GetFirstUnwatchedItemIndex(bool includeAllSeasons, bool includeSpecials)
{
  int iIndex = 0;
  int iUnwatchedSeason = INT_MAX;
  int iUnwatchedEpisode = INT_MAX;
  NODE_TYPE nodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_vecItems->GetPath());

  // Run through the list of items and find the first unwatched season/episode
  for (int i = 0; i < m_vecItems->Size(); ++i)
  {
    CFileItemPtr pItem = m_vecItems->Get(i);
    if (pItem->IsParentFolder() || !pItem->HasVideoInfoTag())
      continue;

    CVideoInfoTag *pTag = pItem->GetVideoInfoTag();

    if ((!includeAllSeasons && pTag->m_iSeason < 0) || (!includeSpecials && pTag->m_iSeason == 0))
      continue;

    // Use the special sort values if they're available
    int iSeason = pTag->m_iSpecialSortSeason >= 0 ? pTag->m_iSpecialSortSeason : pTag->m_iSeason;
    int iEpisode = pTag->m_iSpecialSortEpisode >= 0 ? pTag->m_iSpecialSortEpisode : pTag->m_iEpisode;

    if (nodeType == NODE_TYPE::NODE_TYPE_SEASONS)
    {
      // Is the season unwatched, and is its season number lower than the currently identified
      // first unwatched season
      if (pTag->GetPlayCount() == 0 && iSeason < iUnwatchedSeason)
      {
        iUnwatchedSeason = iSeason;
        iIndex = i;
      }
    }

    if (nodeType == NODE_TYPE::NODE_TYPE_EPISODES)
    {
      // Is the episode unwatched, and is its season number lower
      // or is its episode number lower within the current season
      if (pTag->GetPlayCount() == 0 && (iSeason < iUnwatchedSeason || (iSeason == iUnwatchedSeason && iEpisode < iUnwatchedEpisode)))
      {
        iUnwatchedSeason = iSeason;
        iUnwatchedEpisode = iEpisode;
        iIndex = i;
      }
    }
  }

  return iIndex;
}

bool CGUIWindowVideoNav::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (!CGUIWindowVideoBase::Update(strDirectory, updateFilterPath))
    return false;

  SelectFirstUnwatched();

  return true;
}

void CGUIWindowVideoNav::SelectFirstUnwatched() {
  // Check if we should select the first unwatched item
  SelectFirstUnwatchedItem selectFirstUnwatched = GetSettingSelectFirstUnwatchedItem();
  if (selectFirstUnwatched != SelectFirstUnwatchedItem::NEVER)
  {
    bool bIsItemSelected = (m_viewControl.GetSelectedItem() > 0);

    if (selectFirstUnwatched == SelectFirstUnwatchedItem::ALWAYS ||
      (selectFirstUnwatched == SelectFirstUnwatchedItem::ON_FIRST_ENTRY && !bIsItemSelected))
    {
      IncludeAllSeasonsAndSpecials incAllSeasonsSpecials = GetSettingIncludeAllSeasonsAndSpecials();

      bool bIncludeAllSeasons = (incAllSeasonsSpecials == IncludeAllSeasonsAndSpecials::BOTH || incAllSeasonsSpecials == IncludeAllSeasonsAndSpecials::ALL_SEASONS);
      bool bIncludeSpecials = (incAllSeasonsSpecials == IncludeAllSeasonsAndSpecials::BOTH || incAllSeasonsSpecials == IncludeAllSeasonsAndSpecials::SPECIALS);

      int iIndex = GetFirstUnwatchedItemIndex(bIncludeAllSeasons, bIncludeSpecials);
      m_viewControl.SetSelectedItem(iIndex);
    }
  }
}

bool CGUIWindowVideoNav::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  items.ClearArt();
  items.ClearProperties();

  bool bResult = CGUIWindowVideoBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (VIDEO::IsVideoDb(items))
    {
      XFILE::CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(items.GetPath(),params);
      VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

      int iFlatten = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS);
      int itemsSize = items.GetObjectCount();
      int firstIndex = items.Size() - itemsSize;

      // perform the flattening logic for tvshows with a single (unwatched) season (+ optional special season)
      if (node == NODE_TYPE_SEASONS && !items.IsEmpty())
      {
        // check if the last item is the "All seasons" item which should be ignored for flattening
        if (!items[items.Size() - 1]->HasVideoInfoTag() || items[items.Size() - 1]->GetVideoInfoTag()->m_iSeason < 0)
          itemsSize -= 1;

        bool bFlatten = (itemsSize == 1 && iFlatten == 1) || iFlatten == 2 ||                              // flatten if one one season or if always flatten is enabled
                        (itemsSize == 2 && iFlatten == 1 &&                                                // flatten if one season + specials
                         (items[firstIndex]->GetVideoInfoTag()->m_iSeason == 0 || items[firstIndex + 1]->GetVideoInfoTag()->m_iSeason == 0));

        if (iFlatten > 0 && !bFlatten && (WatchedMode)CMediaSettings::GetInstance().GetWatchedMode("tvshows") == WatchedModeUnwatched)
        {
          int count = 0;
          for(int i = 0; i < items.Size(); i++)
          {
            const CFileItemPtr item = items.Get(i);
            if (item->GetProperty("unwatchedepisodes").asInteger() != 0 && item->GetVideoInfoTag()->m_iSeason > 0)
              count++;
          }
          bFlatten = (count < 2); // flatten if there is only 1 unwatched season (not counting specials)
        }

        if (bFlatten)
        { // flatten if one season or flatten always
          items.Clear();

          CVideoDbUrl videoUrl;
          if (!videoUrl.FromString(items.GetPath()))
            return false;

          videoUrl.AppendPath("-2/");
          return GetDirectory(videoUrl.ToString(), items);
        }
      }

      if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES ||
          node == NODE_TYPE_SEASONS                          ||
          node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
      {
        CLog::Log(LOGDEBUG, "WindowVideoNav::GetDirectory");
        // grab the show thumb
        CVideoInfoTag details;
        m_database.GetTvShowInfo("", details, params.GetTvShowId());
        std::map<std::string, std::string> art;
        if (m_database.GetArtForItem(details.m_iDbId, details.m_type, art))
        {
          items.AppendArt(art, details.m_type);
          items.SetArtFallback("fanart", "tvshow.fanart");
          if (node == NODE_TYPE_SEASONS)
          { // set an art fallback for "thumb"
            if (items.HasArt("tvshow.poster"))
              items.SetArtFallback("thumb", "tvshow.poster");
            else if (items.HasArt("tvshow.banner"))
              items.SetArtFallback("thumb", "tvshow.banner");
          }
        }

        // Grab fanart data
        items.SetProperty("fanart_color1", details.m_fanart.GetColor(0));
        items.SetProperty("fanart_color2", details.m_fanart.GetColor(1));
        items.SetProperty("fanart_color3", details.m_fanart.GetColor(2));

        // save the show description (showplot)
        items.SetProperty("showplot", details.m_strPlot);
        items.SetProperty("showtitle", details.m_strShowTitle);

        // the container folder thumb is the parent (i.e. season or show)
        if (itemsSize && (node == NODE_TYPE_EPISODES || node == NODE_TYPE_RECENTLY_ADDED_EPISODES))
        {
          int seasonID = -1;
          int seasonParam = params.GetSeason();

          // grab all season art when flatten always
          if (seasonParam == -2 && iFlatten == 2)
            seasonParam = -1;

          if (seasonParam >= -1)
            seasonID = m_database.GetSeasonId(details.m_iDbId, seasonParam);
          else
            seasonID = items[firstIndex]->GetVideoInfoTag()->m_iIdSeason;

          CGUIListItem::ArtMap seasonArt;
          if (seasonID > -1 && m_database.GetArtForItem(seasonID, MediaTypeSeason, seasonArt))
          {
            items.AppendArt(seasonArt, MediaTypeSeason);
            // set an art fallback for "thumb"
            if (items.HasArt("season.poster"))
              items.SetArtFallback("thumb", "season.poster");
            else if (items.HasArt("season.banner"))
              items.SetArtFallback("thumb", "season.banner");
          }
        }
      }
      else if (node == NODE_TYPE_TITLE_MOVIES ||
               node == NODE_TYPE_RECENTLY_ADDED_MOVIES)
      {
        if (params.GetSetId() > 0)
        {
          CGUIListItem::ArtMap setArt;
          if (m_database.GetArtForItem(params.GetSetId(), MediaTypeVideoCollection, setArt))
          {
            items.AppendArt(setArt, MediaTypeVideoCollection);
            items.SetArtFallback("fanart", "set.fanart");
            if (items.HasArt("set.poster"))
              items.SetArtFallback("thumb", "set.poster");
          }
        }
      }
    }
    else if (URIUtils::PathEquals(items.GetPath(), "special://videoplaylists/"))
      items.SetContent("playlists");
    else if (!items.IsVirtualDirectoryRoot())
    { // load info from the database
      std::string label;
      if (items.GetLabel().empty() && m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("video"), &label))
        items.SetLabel(label);
      if (!items.IsSourcesPath() && !items.IsLibraryFolder())
        LoadVideoInfo(items, m_database);
    }

    CVideoDbUrl videoUrl;
    if (videoUrl.FromString(items.GetPath()))
    {
      if (items.GetContent() == "tags" && !items.Contains("newtag://" + videoUrl.GetType()))
      {
        const auto newTag{std::make_shared<CFileItem>("newtag://" + videoUrl.GetType(), false)};
        newTag->SetLabel(g_localizeStrings.Get(20462));
        newTag->SetLabelPreformatted(true);
        newTag->SetSpecialSort(SortSpecialOnTop);
        items.Add(newTag);
      }
    }
  }
  return bResult;
}

void CGUIWindowVideoNav::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems->Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->IsParentFolder()) iItems--;
      if (StringUtils::StartsWith(pItem->GetPath(), "/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems->Size() > 2 &&
      StringUtils::StartsWith(m_vecItems->Get(m_vecItems->Size()-1)->GetPath(), "/-1/"))
      iItems--;
  }
  std::string items = StringUtils::Format("{} {}", iItems, g_localizeStrings.Get(127));
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  // set the filter label
  std::string strLabel;

  // "Playlists"
  if (m_vecItems->IsPath("special://videoplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems->IsPlayList())
  {
    // get playlist name from path
    std::string strDummy;
    URIUtils::Split(m_vecItems->GetPath(), strDummy, strLabel);
  }
  else if (m_vecItems->IsPath("sources://video/"))
    strLabel = g_localizeStrings.Get(744);
  // everything else is from a videodb:// path
  else if (VIDEO::IsVideoDb(*m_vecItems))
  {
    CVideoDatabaseDirectory dir;
    dir.GetLabel(m_vecItems->GetPath(), strLabel);
  }
  else
    strLabel = URIUtils::GetFileName(m_vecItems->GetPath());

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  int watchMode = CMediaSettings::GetInstance().GetWatchedMode(m_vecItems->GetContent());
  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + watchMode));

  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNSHOWALL, watchMode != WatchedModeAll);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());

  CONTROL_ENABLE_ON_CONDITION(CONTROL_UPDATE_LIBRARY, !m_vecItems->IsAddonsPath() && !m_vecItems->IsPlugin() && !m_vecItems->IsScript());
}

bool CGUIWindowVideoNav::GetFilteredItems(const std::string &filter, CFileItemList &items)
{
  bool listchanged = CGUIMediaWindow::GetFilteredItems(filter, items);
  listchanged |= ApplyWatchedFilter(items);

  return listchanged;
}

/// \brief Search for names, genres, artists, directors, and plots with search string \e strSearch in the
/// \brief video databases and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoNav::DoSearch(const std::string& strSearch, CFileItemList& items)
{
  CFileItemList tempItems;

  struct Entry
  {
    int id;
    std::function<void()> func;
    int prefix = 0;
  };

  const auto entries = std::array{
      Entry{20338,
            [&strSearch, &tempItems, this]() { m_database.GetMoviesByName(strSearch, tempItems); }},
      Entry{20359, [&strSearch, &tempItems,
                    this]() { m_database.GetEpisodesByName(strSearch, tempItems); }},
      Entry{20364, [&strSearch, &tempItems,
                    this]() { m_database.GetTvShowsByName(strSearch, tempItems); }},
      Entry{20391, [&strSearch, &tempItems,
                    this]() { m_database.GetMusicVideosByName(strSearch, tempItems); }},
      Entry{558, [&strSearch, &tempItems,
                  this]() { m_database.GetMusicVideosByAlbum(strSearch, tempItems); }},
      Entry{20342,
            [&strSearch, &tempItems, this]() {
              m_database.GetMovieGenresByName(strSearch, tempItems);
            },
            515},
      Entry{20343,
            [&strSearch, &tempItems, this]() {
              m_database.GetTvShowGenresByName(strSearch, tempItems);
            },
            515},
      Entry{20389,
            [&strSearch, &tempItems, this]() {
              m_database.GetMusicVideoGenresByName(strSearch, tempItems);
            },
            515},
      Entry{20342,
            [&strSearch, &tempItems, this]() {
              m_database.GetMovieActorsByName(strSearch, tempItems);
            },
            20337},
      Entry{20343,
            [&strSearch, &tempItems, this]() {
              m_database.GetTvShowsActorsByName(strSearch, tempItems);
            },
            20337},
      Entry{20389,
            [&strSearch, &tempItems, this]() {
              m_database.GetMusicVideoArtistsByName(strSearch, tempItems);
            },
            20337},
      Entry{20342,
            [&strSearch, &tempItems, this]() {
              m_database.GetMovieDirectorsByName(strSearch, tempItems);
            },
            20339},
      Entry{20343,
            [&strSearch, &tempItems, this]() {
              m_database.GetTvShowsDirectorsByName(strSearch, tempItems);
            },
            20339},
      Entry{20389,
            [&strSearch, &tempItems, this]() {
              m_database.GetMusicVideoDirectorsByName(strSearch, tempItems);
            },
            20339},
      Entry{20365, [&strSearch, &tempItems,
                    this]() { m_database.GetEpisodesByPlot(strSearch, tempItems); }},
      Entry{20323,
            [&strSearch, &tempItems, this]() { m_database.GetMoviesByPlot(strSearch, tempItems); }},
  };

  std::for_each(entries.begin(), entries.end(), [this, &items, &tempItems](const auto& entry) {
    entry.func();
    std::string msg;
    if (entry.prefix > 0)
    {
      msg = fmt::format("[{} - {}] ", g_localizeStrings.Get(entry.prefix),
                        g_localizeStrings.Get(entry.id));
    }
    else
      msg = fmt::format("[{}] ", g_localizeStrings.Get(entry.id));

    this->AppendAndClearSearchItems(tempItems, msg, items);
  });
}

void CGUIWindowVideoNav::OnDeleteItem(const CFileItemPtr& pItem)
{
  if (m_vecItems->IsParentFolder())
    return;

  if (!VIDEO::IsVideoDb(*m_vecItems) && !VIDEO::IsVideoDb(*pItem))
  {
    if (!pItem->IsPath("newsmartplaylist://video") && !pItem->IsPath("special://videoplaylists/") &&
        !pItem->IsPath("sources://video/") && !URIUtils::IsProtocol(pItem->GetPath(), "newtag"))
      CGUIWindowVideoBase::OnDeleteItem(pItem);
  }
  else if (StringUtils::StartsWithNoCase(pItem->GetPath(), "videodb://movies/sets/") &&
           pItem->GetPath().size() > 22 && pItem->m_bIsFolder)
  {
    CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);

    if (!pDialog)
      return;

    pDialog->SetHeading(CVariant{432});
    std::string strLabel = StringUtils::Format(
        g_localizeStrings.Get(pItem->HasVideoVersions() ? 40021 : 433), pItem->GetLabel());
    pDialog->SetLine(1, CVariant{std::move(strLabel)});
    pDialog->SetLine(2, CVariant{""});
    pDialog->Open();
    if (pDialog->IsConfirmed())
    {
      CFileItemList items;
      CDirectory::GetDirectory(pItem->GetPath(),items,"",DIR_FLAG_NO_FILE_DIRS);
      for (int i=0;i<items.Size();++i)
        OnDeleteItem(items[i]);

      CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(pItem->GetPath(),params);
      m_database.DeleteSet(params.GetSetId());
    }
  }
  else if (m_vecItems->IsPath(CUtil::VideoPlaylistsLocation()) ||
           m_vecItems->IsPath("special://videoplaylists/"))
  {
    pItem->m_bIsFolder = false;
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui && gui->ConfirmDelete(pItem->GetPath()))
      CFileUtils::DeleteItem(pItem);
  }
  else
  {
    if (!CGUIDialogVideoInfo::DeleteVideoItem(pItem))
      return;
  }
  int itemNumber = m_viewControl.GetSelectedItem();
  int select = itemNumber >= m_vecItems->Size()-1 ? itemNumber-1 : itemNumber;
  m_viewControl.SetSelectedItem(select);

  CUtil::DeleteVideoDatabaseDirectoryCache();
}

void CGUIWindowVideoNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);

  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems->GetPath());

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (!item)
  {
    // nothing to do here
  }
  else if (m_vecItems->IsPath("sources://video/"))
  {
    // get the usual shares
    CGUIDialogContextMenu::GetContextButtons("video", item, buttons);
    if (!item->IsDVD() && item->GetPath() != "add" && !item->IsParentFolder() &&
        (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CVideoDatabase database;
      database.Open();
      ADDON::ScraperPtr info = database.GetScraperForPath(item->GetPath());

      if (!item->IsLiveTV() && !item->IsAddonsPath() && !URIUtils::IsUPnP(item->GetPath()))
      {
        if (info && info->Content() != CONTENT_NONE)
        {
          buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }
        else
          buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
      }
    }
  }
  else
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems->IsPath(CUtil::VideoPlaylistsLocation()) ||
                       m_vecItems->IsPath("special://videoplaylists/");

    if (item->HasVideoInfoTag() && item->HasProperty("artist_musicid"))
      buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20396);

    if (item->HasVideoInfoTag() && item->HasProperty("album_musicid"))
      buttons.Add(CONTEXT_BUTTON_GO_TO_ALBUM, 20397);

    if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strAlbum.empty() &&
        !item->GetVideoInfoTag()->m_artist.empty()                              &&
        !item->GetVideoInfoTag()->m_strTitle.empty())
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetSongByArtistAndAlbumAndTitle(StringUtils::Join(item->GetVideoInfoTag()->m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator),
                                                   item->GetVideoInfoTag()->m_strAlbum,
                                                   item->GetVideoInfoTag()->m_strTitle) > -1)
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20398);
      }
    }
    if (!item->IsParentFolder())
    {
      ADDON::ScraperPtr info;
      VIDEO::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      // can we update the database?
      if (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if (!CVideoLibraryQueue::GetInstance().IsScanningLibrary() && VIDEO::IsVideoDb(*item) &&
            item->HasVideoInfoTag() &&
            (item->GetVideoInfoTag()->m_type == MediaTypeMovie || // movies
             item->GetVideoInfoTag()->m_type == MediaTypeTvShow || // tvshows
             item->GetVideoInfoTag()->m_type == MediaTypeSeason || // seasons
             item->GetVideoInfoTag()->m_type == MediaTypeEpisode || // episodes
             item->GetVideoInfoTag()->m_type == MediaTypeMusicVideo || // musicvideos
             item->GetVideoInfoTag()->m_type == "tag" || // tags
             item->GetVideoInfoTag()->m_type == MediaTypeVideoCollection)) // sets
        {
          buttons.Add(CONTEXT_BUTTON_EDIT, 16106);
        }
        if (node == NODE_TYPE_TITLE_TVSHOWS)
        {
          buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }

        if (node == NODE_TYPE_ACTOR && !dir.IsAllItem(item->GetPath()) && item->m_bIsFolder)
        {
          if (StringUtils::StartsWithNoCase(m_vecItems->GetPath(), "videodb://musicvideos")) // mvids
            buttons.Add(CONTEXT_BUTTON_SET_ARTIST_THUMB, 13359);
          else
            buttons.Add(CONTEXT_BUTTON_SET_ACTOR_THUMB, 20403);
        }
      }

      if (!VIDEO::IsVideoDb(*m_vecItems) && !m_vecItems->IsVirtualDirectoryRoot())
      { // non-video db items, file operations are allowed
        if ((CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) &&
            CUtil::SupportsWriteFileOperations(item->GetPath())) ||
            (inPlaylists && URIUtils::GetFileName(item->GetPath()) != "PartyMode-Video.xsp"
                         && (item->IsPlayList() || item->IsSmartPlayList())))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
        // add "Set/Change content" to folders
        if (item->m_bIsFolder && !VIDEO::IsVideoDb(*item) && !item->IsPlayList() &&
            !item->IsSmartPlayList() && !item->IsLibraryFolder() && !item->IsLiveTV() &&
            !item->IsPlugin() && !item->IsAddonsPath() && !URIUtils::IsUPnP(item->GetPath()))
        {
          if (info && info->Content() != CONTENT_NONE)
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          else
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);

          if (info && info->Content() != CONTENT_NONE)
            buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }
      }

      if ((!item->HasVideoInfoTag() || item->GetVideoInfoTag()->m_iDbId == -1) && info && info->Content() != CONTENT_NONE)
        buttons.Add(CONTEXT_BUTTON_SCAN_TO_LIBRARY, 21845);

    }
  }
}

bool CGUIWindowVideoNav::OnPopupMenu(int iItem)
{
  if (iItem >= 0 && iItem < m_vecItems->Size())
  {
    const auto item = m_vecItems->Get(iItem);
    item->SetProperty("CheckAutoPlayNextItem", true);
  }

  return CGUIWindowVideoBase::OnPopupMenu(iItem);
}

bool CGUIWindowVideoNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("video", item, button))
  {
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsLiveTV() 
        &&!item->IsRSS() && !URIUtils::IsUPnP(item->GetPath()))
    {
      // if the source has been properly removed, remove the cached source list because the list has changed
      if (OnUnAssignContent(item->GetPath(), 20375, 20340))
        m_vecItems->RemoveDiscCache(GetID());
    }
    Refresh();
    return true;
  }
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT:
    {
      const CONTEXT_BUTTON ret{
          static_cast<CONTEXT_BUTTON>(CGUIDialogVideoInfo::ManageVideoItem(item))};

      if (ret != CONTEXT_BUTTON_CANCELLED)
      {
        Refresh(true);
        if (ret == CONTEXT_BUTTON_DELETE)
        {
          int select = itemNumber >= m_vecItems->Size()-1 ? itemNumber-1:itemNumber;
          m_viewControl.SetSelectedItem(select);
        }
      }
      return true;
    }

  case CONTEXT_BUTTON_SET_ACTOR_THUMB:
  case CONTEXT_BUTTON_SET_ARTIST_THUMB:
    {
      std::string type = MediaTypeSeason;
      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
        type = "actor";
      else if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
        type = MediaTypeArtist;

      bool result = CGUIDialogVideoInfo::ManageVideoItemArtwork(m_vecItems->Get(itemNumber), type);
      Refresh();

      return result;
    }
  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      std::string strPath;
      strPath = StringUtils::Format("musicdb://artists/{}/",
                                    item->GetProperty("artist_musicid").asInteger());
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_NAV, strPath);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ALBUM:
    {
      std::string strPath;
      strPath = StringUtils::Format("musicdb://albums/{}/",
                                    item->GetProperty("album_musicid").asInteger());
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_NAV, strPath);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CMusicDatabase database;
      database.Open();
      CSong song;
      if (database.GetSong(database.GetSongByArtistAndAlbumAndTitle(StringUtils::Join(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator),m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum,
                                                                        m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strTitle),
                                                                        song))
      {
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0,
                                                   static_cast<void*>(new CFileItem(song)));
      }
      return true;
    }
  case CONTEXT_BUTTON_SCAN_TO_LIBRARY:
    CGUIDialogVideoInfo::ShowFor(*item);
    return true;

  default:
    break;

  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowVideoNav::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("video");
}

bool CGUIWindowVideoNav::OnClick(int iItem, const std::string &player)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (StringUtils::StartsWithNoCase(item->GetPath(), "newtag://"))
  {
    // dont allow update while scanning
    if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
    {
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{14057});
      return true;
    }

    //Get the new title
    std::string strTag;
    if (!CGUIKeyboardFactory::ShowAndGetInput(strTag, CVariant{g_localizeStrings.Get(20462)}, false))
      return true;

    CVideoDatabase videodb;
    if (!videodb.Open())
      return true;

    // get the media type and convert from plural to singular (by removing the trailing "s")
    std::string mediaType = item->GetPath().substr(9);
    mediaType.pop_back();
    std::string localizedType = CGUIDialogVideoInfo::GetLocalizedVideoType(mediaType);
    if (localizedType.empty())
      return true;

    if (!videodb.GetSingleValue("tag", "tag.tag_id", videodb.PrepareSQL("tag.name = '%s' AND tag.tag_id IN (SELECT tag_link.tag_id FROM tag_link WHERE tag_link.media_type = '%s')", strTag.c_str(), mediaType.c_str())).empty())
    {
      std::string strError = StringUtils::Format(g_localizeStrings.Get(20463), strTag);
      HELPERS::ShowOKDialogText(CVariant{20462}, CVariant{std::move(strError)});
      return true;
    }

    int idTag = videodb.AddTag(strTag);
    CFileItemList items;
    std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20464), localizedType);
    if (CGUIDialogVideoInfo::GetItemsForTag(strLabel, mediaType, items, idTag))
    {
      for (int index = 0; index < items.Size(); index++)
      {
        if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
          continue;

        videodb.AddTagToItem(items[index]->GetVideoInfoTag()->m_iDbId, idTag, mediaType);
      }
    }

    Refresh(true);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem, player);
}

std::string CGUIWindowVideoNav::GetStartFolder(const std::string &dir)
{
  static const auto map = std::map<std::string, std::string>{
      {"files", "sources://video/"},
      {"inprogresstvshows", "videodb://inprogresstvshows/"},
      {"movieactors", "videodb://movies/actors/"},
      {"moviecountries", "videodb://movies/countries/"},
      {"moviedirectors", "videodb://movies/directors/"},
      {"moviegenres", "videodb://movies/genres/"},
      {"movies", "videodb://movies/"},
      {"moviesets", "videodb://movies/sets/"},
      {"moviestudios", "videodb://movies/studios/"},
      {"movietags", "videodb://movies/tags/"},
      {"movietitles", "videodb://movies/titles/"},
      {"movieyears", "videodb://movies/years/"},
      {"musicvideoalbums", "videodb://musicvideos/albums/"},
      {"musicvideoartists", "videodb://musicvideos/artists/"},
      {"musicvideodirectors", "videodb://musicvideos/directors/"},
      {"musicvideogenres", "videodb://musicvideos/genres/"},
      {"musicvideos", "videodb://musicvideos/"},
      {"musicvideostudios", "videodb://musicvideos/studios/"},
      {"musicvideotags", "videodb://musicvideos/tags/"},
      {"musicvideotitles", "videodb://musicvideos/titles/"},
      {"musicvideoyears", "videodb://musicvideos/years/"},
      {"recentlyaddedepisodes", "videodb://recentlyaddedepisodes/"},
      {"recentlyaddedmovies", "videodb://recentlyaddedmovies/"},
      {"recentlyaddedmusicvideos", "videodb://recentlyaddedmusicvideos/"},
      {"tvshowactors", "videodb://tvshows/actors/"},
      {"tvshowgenres", "videodb://tvshows/genres/"},
      {"tvshows", "videodb://tvshows/"},
      {"tvshowstudios", "videodb://tvshows/studios/"},
      {"tvshowtags", "videodb://tvshows/tags/"},
      {"tvshowtitles", "videodb://tvshows/titles/"},
      {"tvshowyears", "videodb://tvshows/years/"},
  };

  const auto it = map.find(StringUtils::ToLower(dir));
  if (it == map.end())
    return CGUIWindowVideoBase::GetStartFolder(dir);
  else
    return it->second;
}

bool CGUIWindowVideoNav::ApplyWatchedFilter(CFileItemList &items)
{
  bool listchanged = false;
  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

  // now filter watched items as necessary
  bool filterWatched=false;
  if (node == NODE_TYPE_EPISODES
  ||  node == NODE_TYPE_SEASONS
  ||  node == NODE_TYPE_SETS
  ||  node == NODE_TYPE_TAGS
  ||  node == NODE_TYPE_TITLE_MOVIES
  ||  node == NODE_TYPE_TITLE_TVSHOWS
  ||  node == NODE_TYPE_TITLE_MUSICVIDEOS
  ||  node == NODE_TYPE_RECENTLY_ADDED_EPISODES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MOVIES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
    filterWatched = true;
  if (!VIDEO::IsVideoDb(items))
    filterWatched = true;
  if (items.GetContent() == "tvshows" &&
     (items.IsSmartPlayList() || items.IsLibraryFolder()))
    node = NODE_TYPE_TITLE_TVSHOWS; // so that the check below works

  int watchMode = CMediaSettings::GetInstance().GetWatchedMode(m_vecItems->GetContent());

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);

    if(item->HasVideoInfoTag() && (node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS))
    {
      if (watchMode == WatchedModeUnwatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("unwatchedepisodes").asInteger();
      if (watchMode == WatchedModeWatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("watchedepisodes").asInteger();
      if (watchMode == WatchedModeAll)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("totalepisodes").asInteger();
      item->SetProperty("numepisodes", item->GetVideoInfoTag()->m_iEpisode);
      listchanged = true;
    }

    if (filterWatched)
    {
      if(!item->IsParentFolder() && // Don't delete the go to parent folder
         ((watchMode == WatchedModeWatched   && item->GetVideoInfoTag()->GetPlayCount() == 0) ||
          (watchMode == WatchedModeUnwatched && item->GetVideoInfoTag()->GetPlayCount() > 0)))
      {
        items.Remove(i);
        i--;
        listchanged = true;
      }
    }
  }

  // Remove the parent folder icon, if it's the only thing in the folder. This is needed for hiding seasons.
  if (items.GetObjectCount() == 0 && items.GetFileCount() > 0 && items.Get(0)->IsParentFolder())
      items.Remove(0);

  if(node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS)
  {
    // the watched filter may change the "numepisodes" property which is reflected in the TV_SHOWS and SEASONS nodes
    // therefore, the items labels have to be refreshed, and possibly the list needs resorting as well.
    items.ClearSortState(); // this is needed to force resorting even if sort method did not change
    FormatAndSort(items);
  }

  return listchanged;
}
