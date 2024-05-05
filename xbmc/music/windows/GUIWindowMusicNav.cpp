/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowMusicNav.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonSystemSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicLibraryQueue.h"
#include "music/dialogs/GUIDialogInfoProviderSettings.h"
#include "music/tags/MusicInfoTag.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "view/GUIViewState.h"

using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSICDATABASEDIRECTORY;
using namespace KODI;
using namespace KODI::MESSAGING;
using namespace KODI::VIDEO;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LABELFILES        12

#define CONTROL_SEARCH             8
#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_BTNMANUALINFO     17
#define CONTROL_BTN_FILTER        19

#define CONTROL_UPDATE_LIBRARY    20

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_NAV, "MyMusicNav.xml")
{
  m_vecItems->SetPath("?");
  m_searchWithEdit = false;
}

CGUIWindowMusicNav::~CGUIWindowMusicNav(void) = default;

bool CGUIWindowMusicNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("?");
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      /* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);

      // is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW));

      if (!CGUIWindowMusicBase::OnMessage(message))
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
              OnItemInfo(i);
            break;
          }
        }
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
          if (!g_partyModeManager.Enable())
          {
            SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE,false);
            return false;
          }

          // Playlist directory is the root of the playlist window
          if (m_guiState)
            m_guiState->SetPlaylistDirectory("playlistmusic://");

          return true;
        }
        UpdateButtons();
      }
      else if (iControl == CONTROL_SEARCH)
      {
        if (m_searchWithEdit)
        {
          // search updated - reset timer
          m_searchTimer.StartZero();
          // grab our search string
          CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SEARCH);
          OnMessage(selected);
          SetProperty("search", selected.GetLabel());
          return true;
        }
        std::string search(GetProperty("search").asString());
        CGUIKeyboardFactory::ShowAndGetFilter(search, true);
        SetProperty("search", search);
        return true;
      }
      else if (iControl == CONTROL_UPDATE_LIBRARY)
      {
        if (!CMusicLibraryQueue::GetInstance().IsScanningLibrary())
          CMusicLibraryQueue::GetInstance().ScanLibrary("");
        else
          CMusicLibraryQueue::GetInstance().StopLibraryScanning();
        return true;
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
  case GUI_MSG_PLAYBACK_STARTED:
    {
      SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_SEARCH_UPDATE && IsActive())
      {
        // search updated - reset timer
        m_searchTimer.StartZero();
        SetProperty("search", message.GetStringParam());
      }
    }
  }
  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicNav::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_SCAN_ITEM)
  {
    int item = m_viewControl.GetSelectedItem();
    CMusicDatabaseDirectory dir;
    if (item > -1 && m_vecItems->Get(item)->m_bIsFolder
                  && (m_vecItems->Get(item)->IsAlbum()||
                      dir.IsArtistDir(m_vecItems->Get(item)->GetPath())))
    {
      OnContextButton(item,CONTEXT_BUTTON_INFO);
      return true;
    }
  }

  return CGUIWindowMusicBase::OnAction(action);
}

bool CGUIWindowMusicNav::ManageInfoProvider(const CFileItemPtr& item)
{
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(item->GetPath(), params);
  // Management of Info provider only valid for specific artist or album items
  if (params.GetAlbumId() == -1 && params.GetArtistId() == -1)
    return false;

  // Set things up for processing artist or albums
  CONTENT_TYPE content = CONTENT_ALBUMS;
  int id = params.GetAlbumId();
  if (id == -1)
  {
    content = CONTENT_ARTISTS;
    id = params.GetArtistId();
  }

  ADDON::ScraperPtr scraper;
  // Get specific scraper and settings for current  item or use default
  if (!m_musicdatabase.GetScraper(id, content, scraper))
  {
    ADDON::AddonPtr defaultScraper;
    if (ADDON::CAddonSystemSettings::GetInstance().GetActive(
        ADDON::ScraperTypeFromContent(content), defaultScraper))
    {
      scraper = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
    }
  }

  // Set Information provider and settings
  int applyto = CGUIDialogInfoProviderSettings::Show(scraper);
  if (applyto >= 0)
  {
    bool result = false;
    CVariant msgctxt;
    switch (applyto)
    {
    case INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_THISITEM: // Change information provider for specific item
      result = m_musicdatabase.SetScraper(id, content, scraper);
      break;
    case INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_ALLVIEW: // Change information provider for the filtered items shown on this node
      {
        msgctxt = 38069;
        if (content == CONTENT_ARTISTS)
          msgctxt = 38068;
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{ 20195 }, msgctxt)) // Change information provider, confirm for all shown
        {
          // Set scraper for all items on current view.
          std::string strPath = "musicdb://";
          if (content == CONTENT_ARTISTS)
            strPath += "artists";
          else
            strPath += "albums";
          URIUtils::AddSlashAtEnd(strPath);
          // Items on view could be limited by navigation criteria, smart playlist rules or a filter.
          // Get these options, except ID, from item path
          CURL musicUrl(item->GetPath());  //Use CURL, as CMusicDbUrl removes "filter" option
          if (content == CONTENT_ARTISTS)
            musicUrl.RemoveOption("artistid");
          else
            musicUrl.RemoveOption("albumid");
         strPath += musicUrl.GetOptions();
          result = m_musicdatabase.SetScraperAll(strPath, scraper);
        }
      }
      break;
    case INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_DEFAULT: // Change information provider for all items
      {
        msgctxt = 38071;
        if (content == CONTENT_ARTISTS)
          msgctxt = 38070;
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20195}, msgctxt)) // Change information provider, confirm default and clear
        {
          // Save scraper addon default setting values
          scraper->SaveSettings();
          // Set default scraper
          const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
          if (content == CONTENT_ARTISTS)
            settings->SetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, scraper->ID());
          else
            settings->SetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, scraper->ID());
          settings->Save();
          // Clear all item specific settings
          if (content == CONTENT_ARTISTS)
            result = m_musicdatabase.SetScraperAll("musicdb://artists/", nullptr);
          else
            result = m_musicdatabase.SetScraperAll("musicdb://albums/", nullptr);
        }
      }
    default:
      break;
    }
    if (!result)
      return false;

    // Refresh additional information using the new settings
    if (applyto == INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_ALLVIEW || applyto == INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_DEFAULT)
    {
      // Change information provider, all artists or albums
      if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20195}, CVariant{38072}))
        OnItemInfoAll(m_vecItems->GetPath(), true);
    }
    else
    {
      // Change information provider, selected artist or album
      if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20195}, CVariant{38073}))
      {
        std::string itempath = StringUtils::Format("musicdb://albums/{}/", id);
        if (content == CONTENT_ARTISTS)
          itempath = StringUtils::Format("musicdb://artists/{}/", id);
        OnItemInfoAll(itempath, true);
      }
    }
  }
  return true;
}

bool CGUIWindowMusicNav::OnClick(int iItem, const std::string &player /* = "" */)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return false;

  CFileItemPtr item = m_vecItems->Get(iItem);
  if (StringUtils::StartsWith(item->GetPath(), "musicsearch://"))
  {
    if (m_searchWithEdit)
      OnSearchUpdate();
    else
    {
      std::string search(GetProperty("search").asString());
      CGUIKeyboardFactory::ShowAndGetFilter(search, true);
      SetProperty("search", search);
    }
    return true;
  }
  if (MUSIC::IsMusicDb(*item) && !item->m_bIsFolder)
    m_musicdatabase.SetPropertiesForFileItem(*item);

  if (item->IsPlayList() &&
    !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders)
  {
    PlayItem(iItem);
    return true;
  }
  return CGUIWindowMusicBase::OnClick(iItem, player);
}

bool CGUIWindowMusicNav::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (CGUIWindowMusicBase::Update(strDirectory, updateFilterPath))
  {
    m_thumbLoader.Load(*m_unfilteredItems);
    return true;
  }

  return false;
}

bool CGUIWindowMusicNav::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  if (strDirectory.empty())
    AddSearchFolder();

  bool bResult = CGUIWindowMusicBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsPlayList())
      OnRetrieveMusicInfo(items);
  }

  // update our content in the info manager
  if (StringUtils::StartsWithNoCase(strDirectory, "videodb://") || IsVideoDb(items))
  {
    CVideoDatabaseDirectory dir;
    VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());
    switch (node)
    {
      case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MUSICVIDEOS:
      case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
        items.SetContent("musicvideos");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_GENRE:
        items.SetContent("genres");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_COUNTRY:
        items.SetContent("countries");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_ACTOR:
        items.SetContent("artists");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_DIRECTOR:
        items.SetContent("directors");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_STUDIO:
        items.SetContent("studios");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_YEAR:
        items.SetContent("years");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_ALBUM:
        items.SetContent("albums");
        break;
      case VIDEODATABASEDIRECTORY::NODE_TYPE_TAGS:
        items.SetContent("tags");
        break;
      default:
        items.SetContent("");
        break;
    }
  }
  else if (StringUtils::StartsWithNoCase(strDirectory, "musicdb://") || MUSIC::IsMusicDb(items))
  {
    CMusicDatabaseDirectory dir;
    NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());
    switch (node)
    {
      case NODE_TYPE_ALBUM:
      case NODE_TYPE_ALBUM_RECENTLY_ADDED:
      case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
      case NODE_TYPE_ALBUM_TOP100:
      case NODE_TYPE_DISC: // ! @todo: own content type "discs"??
        items.SetContent("albums");
        break;
      case NODE_TYPE_ARTIST:
        items.SetContent("artists");
        break;
      case NODE_TYPE_SONG:
      case NODE_TYPE_SONG_TOP100:
      case NODE_TYPE_SINGLES:
      case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
      case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
      case NODE_TYPE_ALBUM_TOP100_SONGS:
        items.SetContent("songs");
        break;
      case NODE_TYPE_GENRE:
        items.SetContent("genres");
        break;
      case NODE_TYPE_SOURCE:
        items.SetContent("sources");
        break;
      case NODE_TYPE_ROLE:
        items.SetContent("roles");
        break;
      case NODE_TYPE_YEAR:
        items.SetContent("years");
        break;
      default:
        items.SetContent("");
        break;
    }
  }
  else if (items.IsPlayList())
    items.SetContent("songs");
  else if (URIUtils::PathEquals(strDirectory, "special://musicplaylists/") ||
           URIUtils::PathEquals(strDirectory, "library://music/playlists.xml/"))
    items.SetContent("playlists");
  else if (URIUtils::PathEquals(strDirectory, "plugin://music/"))
    items.SetContent("plugins");
  else if (items.IsAddonsPath())
    items.SetContent("addons");
  else if (!items.IsSourcesPath() && !items.IsVirtualDirectoryRoot() &&
           !items.IsLibraryFolder() && !items.IsPlugin() && !items.IsSmartPlayList())
    items.SetContent("files");

  return bResult;
}

void CGUIWindowMusicNav::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

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
  if (m_vecItems->IsPath("special://musicplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems->IsPlayList())
  {
    // get playlist name from path
    std::string strDummy;
    URIUtils::Split(m_vecItems->GetPath(), strDummy, strLabel);
  }
  // everything else is from a musicdb:// path
  else
  {
    CMusicDatabaseDirectory dir;
    dir.GetLabel(m_vecItems->GetPath(), strLabel);
  }

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());

  CONTROL_ENABLE_ON_CONDITION(CONTROL_UPDATE_LIBRARY, !m_vecItems->IsAddonsPath() && !m_vecItems->IsPlugin() && !m_vecItems->IsScript());
}

void CGUIWindowMusicNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems->IsVirtualDirectoryRoot() && !m_vecItems->Get(iItem)->IsDVD())
    return;

  CGUIWindowMusicBase::PlayItem(iItem);
}

void CGUIWindowMusicNav::OnWindowLoaded()
{
  const CGUIControl *control = GetControl(CONTROL_SEARCH);
  m_searchWithEdit = (control && control->GetControlType() == CGUIControl::GUICONTROL_EDIT);

  CGUIWindowMusicBase::OnWindowLoaded();

  if (m_searchWithEdit)
  {
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SEARCH, CGUIEditControl::INPUT_TYPE_SEARCH);
    SET_CONTROL_LABEL2(CONTROL_SEARCH, GetProperty("search").asString());
  }
}

void CGUIWindowMusicNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (item)
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    // are we in the playlists location?
    bool inPlaylists = m_vecItems->IsPath(CUtil::MusicPlaylistsLocation()) ||
      m_vecItems->IsPath("special://musicplaylists/");

    if (m_vecItems->IsPath("sources://music/"))
    {
      // get the usual music shares, and anything for all media windows
      CGUIDialogContextMenu::GetContextButtons("music", item, buttons);
#ifdef HAS_OPTICAL_DRIVE
      // enable Rip CD an audio disc
      if (CServiceBroker::GetMediaManager().IsDiscInDrive() && MUSIC::IsCDDA(*item))
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        MEDIA_DETECT::CCdInfo* pCdInfo = CServiceBroker::GetMediaManager().GetCdInfo();
        if (pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1))
        {
          if (CServiceBroker::GetJobManager()->IsProcessing("cdrip"))
            buttons.Add(CONTEXT_BUTTON_CANCEL_RIP_CD, 14100);
          else
            buttons.Add(CONTEXT_BUTTON_RIP_CD, 600);
        }
      }
#endif
      // Scan button for music sources except  ".." and "Add music source" items
      if (!item->IsPath("add") && !item->IsParentFolder() &&
        (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        buttons.Add(CONTEXT_BUTTON_SCAN, 13352);
      }
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
    }
    else
    {
      CGUIWindowMusicBase::GetContextButtons(itemNumber, buttons);

      // Scan button for real folders containing files when navigating within music sources.
      // Blacklist the bespoke Kodi protocols as to many valid external protocols to whitelist
      if (m_vecItems->GetContent() == "files" && // Other content not scanned to library
          !inPlaylists &&
          !NETWORK::IsInternetStream(*m_vecItems) && // Not playlists locations or streams
          !item->IsPath("add") && !item->IsParentFolder() && // Not ".." and "Add items
          item->m_bIsFolder && // Folders only, but playlists can be folders too
          !URIUtils::IsLibraryContent(item->GetPath()) && // database folder or .xsp files
          !URIUtils::IsSpecial(item->GetPath()) && !item->IsPlugin() && !item->IsScript() &&
          !item->IsPlayList() && // .m3u etc. that as flagged as folders when playlistasfolders
          !StringUtils::StartsWithNoCase(item->GetPath(), "addons://") &&
          (profileManager->GetCurrentProfile().canWriteDatabases() ||
           g_passwordManager.bMasterUser))
      {
        buttons.Add(CONTEXT_BUTTON_SCAN, 13352);
      }

      CMusicDatabaseDirectory dir;

      if (!item->IsParentFolder() && !dir.IsAllItem(item->GetPath()))
      {
        if (item->m_bIsFolder && !IsVideoDb(*item) && !item->IsPlugin() &&
            !StringUtils::StartsWithNoCase(item->GetPath(), "musicsearch://"))
        {
          if (item->IsAlbum())
            // enable query all albums button only in album view
            buttons.Add(CONTEXT_BUTTON_INFO_ALL, 20059);
          else if (dir.IsArtistDir(item->GetPath()))
            // enable query all artist button only in artist view
            buttons.Add(CONTEXT_BUTTON_INFO_ALL, 21884);

          //Set default or clear default
          NODE_TYPE nodetype = dir.GetDirectoryType(item->GetPath());
          if (!inPlaylists &&
             (nodetype == NODE_TYPE_ROOT ||
              nodetype == NODE_TYPE_OVERVIEW ||
              nodetype == NODE_TYPE_TOP100))
          {
            const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
            if (!item->IsPath(settings->GetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW)))
              buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // set default
            if (!settings->GetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW).empty())
              buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // clear default
          }

          //Change information provider
          if (StringUtils::EqualsNoCase(m_vecItems->GetContent(), "albums") ||
              StringUtils::EqualsNoCase(m_vecItems->GetContent(), "artists"))
          {
            // we allow the user to set information provider for albums and artists
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20195);
          }
        }
        if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetArtistString().empty())
        {
          CVideoDatabase database;
          database.Open();
          if (database.GetMatchingMusicVideo(item->GetMusicInfoTag()->GetArtistString()) > -1)
            buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20400);
        }
        if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetArtistString().empty() &&
          !item->GetMusicInfoTag()->GetAlbum().empty() &&
          !item->GetMusicInfoTag()->GetTitle().empty())
        {
          CVideoDatabase database;
          database.Open();
          if (database.GetMatchingMusicVideo(item->GetMusicInfoTag()->GetArtistString(), item->GetMusicInfoTag()->GetAlbum(), item->GetMusicInfoTag()->GetTitle()) > -1)
            buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20401);
        }
        if (item->HasVideoInfoTag() && !item->m_bIsFolder)
        {
          if ((profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) && !item->IsPlugin())
          {
            buttons.Add(CONTEXT_BUTTON_RENAME, 16105);
            buttons.Add(CONTEXT_BUTTON_DELETE, 646);
          }
        }
        if (inPlaylists && URIUtils::GetFileName(item->GetPath()) != "PartyMode.xsp"
          && (item->IsPlayList() || item->IsSmartPlayList()))
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);

        if (!item->IsReadOnly() && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("filelists.allowfiledeletion"))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
      }
    }
  }
  // noncontextual buttons

  CGUIWindowMusicBase::GetNonContextButtons(buttons);
}

bool CGUIWindowMusicNav::OnPopupMenu(int iItem)
{
  if (iItem >= 0 && iItem < m_vecItems->Size())
  {
    const auto item = m_vecItems->Get(iItem);
    item->SetProperty("CheckAutoPlayNextItem", true);
  }

  return CGUIWindowMusicBase::OnPopupMenu(iItem);
}

bool CGUIWindowMusicNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  switch (button)
  {
  case CONTEXT_BUTTON_INFO:
    {
      if (!IsVideoDb(*item))
        return CGUIWindowMusicBase::OnContextButton(itemNumber,button);

      // music videos - artists
      if (StringUtils::StartsWithNoCase(item->GetPath(), "videodb://musicvideos/artists/"))
      {
        int idArtist = m_musicdatabase.GetArtistByName(item->GetLabel());
        if (idArtist == -1)
          return false;
        std::string path = StringUtils::Format("musicdb://artists/{}/", idArtist);
        CArtist artist;
        m_musicdatabase.GetArtist(idArtist, artist, false);
        *item = CFileItem(artist);
        item->SetPath(path);
        CGUIWindowMusicBase::OnContextButton(itemNumber,button);
        Refresh();
        m_viewControl.SetSelectedItem(itemNumber);
        return true;
      }

      // music videos - albums
      if (StringUtils::StartsWithNoCase(item->GetPath(), "videodb://musicvideos/albums/"))
      {
        int idAlbum = m_musicdatabase.GetAlbumByName(item->GetLabel());
        if (idAlbum == -1)
          return false;
        std::string path = StringUtils::Format("musicdb://albums/{}/", idAlbum);
        CAlbum album;
        m_musicdatabase.GetAlbum(idAlbum, album, false);
        *item = CFileItem(path,album);
        item->SetPath(path);
        CGUIWindowMusicBase::OnContextButton(itemNumber,button);
        Refresh();
        m_viewControl.SetSelectedItem(itemNumber);
        return true;
      }

      if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strTitle.empty())
      {
        CGUIDialogVideoInfo::ShowFor(*item);
        Refresh();
      }
      return true;
    }

  case CONTEXT_BUTTON_INFO_ALL:
    OnItemInfoAll(m_vecItems->GetPath());
    return true;

  case CONTEXT_BUTTON_SET_DEFAULT:
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW, item->GetPath());
    settings->Save();
    return true;
  }

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW, "");
    settings->Save();
    return true;
  }

  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      std::string strPath;
      CVideoDatabase database;
      database.Open();
      strPath = StringUtils::Format(
          "videodb://musicvideos/artists/{}/",
          database.GetMatchingMusicVideo(item->GetMusicInfoTag()->GetArtistString()));
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV,strPath);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CVideoDatabase database;
      database.Open();
      CVideoInfoTag details;
      database.GetMusicVideoInfo("", details, database.GetMatchingMusicVideo(item->GetMusicInfoTag()->GetArtistString(), item->GetMusicInfoTag()->GetAlbum(), item->GetMusicInfoTag()->GetTitle()));
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0,
                                                 static_cast<void*>(new CFileItem(details)));
      return true;
    }

  case CONTEXT_BUTTON_RENAME:
    if (!IsVideoDb(*item) && !item->IsReadOnly())
      OnRenameItem(itemNumber);

    CGUIDialogVideoInfo::UpdateVideoItemTitle(item);
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Refresh();
    return true;

  case CONTEXT_BUTTON_DELETE:
    if (item->IsPlayList() || item->IsSmartPlayList())
    {
      item->m_bIsFolder = false;
      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui && gui->ConfirmDelete(item->GetPath()))
        CFileUtils::DeleteItem(item);
    }
    else if (!IsVideoDb(*item))
      OnDeleteItem(itemNumber);
    else
    {
      CGUIDialogVideoInfo::DeleteVideoItemFromDatabase(item);
      CUtil::DeleteVideoDatabaseDirectoryCache();
    }
    Refresh();
    return true;

  case CONTEXT_BUTTON_SET_CONTENT:
    return ManageInfoProvider(item);

  default:
    break;
  }

  return CGUIWindowMusicBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowMusicNav::GetSongsFromPlayList(const std::string& strPlayList, CFileItemList &items)
{
  std::string strParentPath=m_history.GetParentPath();

  if (m_guiState.get() && !m_guiState->HideParentDirItems())
  {
    CFileItemPtr pItem(new CFileItem(".."));
    pItem->SetPath(strParentPath);
    items.Add(pItem);
  }

  items.SetPath(strPlayList);
  CLog::Log(LOGDEBUG, "CGUIWindowMusicNav, opening playlist [{}]", strPlayList);

  std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if (nullptr != pPlayList)
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      HELPERS::ShowOKDialogText(CVariant{6}, CVariant{477});
      return false; //hmmm unable to load playlist?
    }
    CPlayList playlist = *pPlayList;
    // convert playlist items to songs
    for (int i = 0; i < playlist.size(); ++i)
    {
      items.Add(playlist[i]);
    }
  }

  return true;
}

void CGUIWindowMusicNav::OnSearchUpdate()
{
  std::string search(CURL::Encode(GetProperty("search").asString()));
  if (!search.empty())
  {
    std::string path = "musicsearch://" + search + "/";
    m_history.ClearSearchHistory();
    Update(path);
  }
  else if (m_vecItems->IsVirtualDirectoryRoot())
  {
    Update("");
  }
}

void CGUIWindowMusicNav::FrameMove()
{
  static const int search_timeout = 2000;
  // update our searching
  if (m_searchTimer.IsRunning() && m_searchTimer.GetElapsedMilliseconds() > search_timeout)
  {
    m_searchTimer.Stop();
    OnSearchUpdate();
  }
  CGUIWindowMusicBase::FrameMove();
}

void CGUIWindowMusicNav::AddSearchFolder()
{
  // we use a general viewstate (and not our member) here as our
  // current viewstate may be specific to some other folder, and
  // we know we're in the root here
  CFileItemList items;
  CGUIViewState* viewState = CGUIViewState::GetViewState(GetID(), items);
  if (viewState)
  {
    // add our remove the musicsearch source
    VECSOURCES &sources = viewState->GetSources();
    bool haveSearchSource = false;
    bool needSearchSource = !GetProperty("search").empty() || !m_searchWithEdit; // we always need it if we don't have the edit control
    for (IVECSOURCES it = sources.begin(); it != sources.end(); ++it)
    {
      CMediaSource& share = *it;
      if (share.strPath == "musicsearch://")
      {
        haveSearchSource = true;
        if (!needSearchSource)
        { // remove it
          sources.erase(it);
          break;
        }
      }
    }
    if (!haveSearchSource && needSearchSource)
    {
      // add search share
      CMediaSource share;
      share.strName=g_localizeStrings.Get(137); // Search
      share.strPath = "musicsearch://";
      share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      sources.push_back(share);
    }
    m_rootDir.SetSources(sources);
    delete viewState;
  }
}

std::string CGUIWindowMusicNav::GetStartFolder(const std::string &dir)
{
  static const auto map = std::map<std::string, std::string>{
      {"albums", "musicdb://albums/"},
      {"artists", "musicdb://artists/"},
      {"boxsets", "musicdb://boxsets/"},
      {"compilations", "musicdb://compilations/"},
      {"files", "sources://music/"},
      {"genres", "musicdb://genres/"},
      {"recentlyaddedalbums", "musicdb://recentlyaddedalbums/"},
      {"recentlyplayedalbums", "musicdb://recentlyplayedalbums/"},
      {"singles", "musicdb://singles/"},
      {"songs", "musicdb://songs/"},
      {"top100", "musicdb://top100/"},
      {"top100albums", "musicdb://top100/albums/"},
      {"top100songs", "musicdb://top100/songs/"},
      {"years", "musicdb://years/"},
  };

  const auto it = map.find(StringUtils::ToLower(dir));
  if (it == map.end())
    return CGUIWindowMusicBase::GetStartFolder(dir);
  else
    return it->second;
}
