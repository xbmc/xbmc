/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIWindowMusicNav.h"
#include "addons/AddonManager.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "PartyModeManager.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "profiles/ProfilesManager.h"
#include "video/VideoDatabase.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/Key.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIEditControl.h"
#include "GUIUserMessages.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "TextureCache.h"
#include "Util.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSICDATABASEDIRECTORY;

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
#define CONTROL_LABELEMPTY        18

#define CONTROL_UPDATE_LIBRARY    20

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_NAV, "MyMusicNav.xml")
{
  m_vecItems->SetPath("?");
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
  m_searchWithEdit = false;
}

CGUIWindowMusicNav::~CGUIWindowMusicNav(void)
{
}

bool CGUIWindowMusicNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("?");
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
/* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);

      // is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CSettings::Get().GetString("mymusic.defaultlibview"));
      
      DisplayEmptyDatabaseMessage(false); // reset message state

      if (!CGUIWindowMusicBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      DisplayEmptyDatabaseMessage(m_musicdatabase.GetSongsCount() <= 0);

      if (m_bDisplayEmptyDatabaseMessage)
      {
        // no library - make sure we focus on a known control, and default to the root.
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        m_vecItems->SetPath("");
        SetHistoryForPath("");
        Update("");
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
          if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistmusic://");

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
        if (!g_application.IsMusicScanning())
          g_application.StartMusicScan("");
        else
          g_application.StopMusicScan();
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
                  && (dir.HasAlbumInfo(m_vecItems->Get(item)->GetPath())||
                      dir.IsArtistDir(m_vecItems->Get(item)->GetPath())))
      OnContextButton(item,CONTEXT_BUTTON_INFO);

    return true;
  }

  return CGUIWindowMusicBase::OnAction(action);
}

std::string CGUIWindowMusicNav::GetQuickpathName(const std::string& strPath) const
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  StringUtils::ToLower(path);
  if (path == "musicdb://genres/")
    return "Genres";
  else if (path == "musicdb://artists/")
    return "Artists";
  else if (path == "musicdb://albums/")
    return "Albums";
  else if (path == "musicdb://songs/")
    return "Songs";
  else if (path == "musicdb://top100/")
    return "Top100";
  else if (path == "musicdb://top100/songs/")
    return "Top100Songs";
  else if (path == "musicdb://top100/albums/")
    return "Top100Albums";
  else if (path == "musicdb://recentlyaddedalbums/")
    return "RecentlyAddedAlbums";
  else if (path == "musicdb://recentlyplayedalbums/")
    return "RecentlyPlayedAlbums";
  else if (path == "musicdb://compilations/")
    return "Compilations";
  else if (path == "musicdb://years/")
    return "Years";
  else if (path == "musicdb://singles/")
    return "Singles";
  else if (path == "special://musicplaylists/")
    return "Playlists";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowMusicNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowMusicNav::OnClick(int iItem)
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
  if (item->IsMusicDb() && !item->m_bIsFolder)
    m_musicdatabase.SetPropertiesForFileItem(*item);
    
  return CGUIWindowMusicBase::OnClick(iItem);
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
  if (m_bDisplayEmptyDatabaseMessage)
    return true;

  if (strDirectory.empty())
    AddSearchFolder();

  bool bResult = CGUIWindowMusicBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsPlayList())
      OnRetrieveMusicInfo(items);
  }

  // update our content in the info manager
  if (StringUtils::StartsWithNoCase(strDirectory, "videodb://"))
  {
    CVideoDatabaseDirectory dir;
    VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(strDirectory);
    if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MUSICVIDEOS ||
        node == VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
      items.SetContent("musicvideos");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_GENRE)
      items.SetContent("genres");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_COUNTRY)
      items.SetContent("countries");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_ACTOR)
      items.SetContent("artists");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_DIRECTOR)
      items.SetContent("directors");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_STUDIO)
      items.SetContent("studios");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_YEAR)
      items.SetContent("years");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_ALBUM)
      items.SetContent("albums");
    else if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_TAGS)
      items.SetContent("tags");
  }
  else if (StringUtils::StartsWithNoCase(strDirectory, "musicdb://"))
  {
    CMusicDatabaseDirectory dir;
    NODE_TYPE node = dir.GetDirectoryChildType(strDirectory);
    if (node == NODE_TYPE_ALBUM ||
        node == NODE_TYPE_ALBUM_RECENTLY_ADDED ||
        node == NODE_TYPE_ALBUM_RECENTLY_PLAYED ||
        node == NODE_TYPE_ALBUM_TOP100 ||
        node == NODE_TYPE_ALBUM_COMPILATIONS ||
        node == NODE_TYPE_YEAR_ALBUM)
      items.SetContent("albums");
    else if (node == NODE_TYPE_ARTIST)
      items.SetContent("artists");
    else if (node == NODE_TYPE_SONG ||
             node == NODE_TYPE_SONG_TOP100 ||
             node == NODE_TYPE_SINGLES ||
             node == NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS ||
             node == NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS ||
             node == NODE_TYPE_ALBUM_COMPILATIONS_SONGS ||
             node == NODE_TYPE_ALBUM_TOP100_SONGS ||
             node == NODE_TYPE_YEAR_SONG)
      items.SetContent("songs");
    else if (node == NODE_TYPE_GENRE)
      items.SetContent("genres");
    else if (node == NODE_TYPE_YEAR)
      items.SetContent("years");
  }
  else if (URIUtils::PathEquals(strDirectory, "special://musicplaylists/"))
    items.SetContent("playlists");
  else if (URIUtils::PathEquals(strDirectory, "plugin://music/"))
    items.SetContent("plugins");
  else if (items.IsPlayList())
    items.SetContent("songs");

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
  std::string items = StringUtils::Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
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
  if (m_vecItems->IsVirtualDirectoryRoot())
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
  CGUIWindowMusicBase::GetContextButtons(itemNumber, buttons);

  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (item && !StringUtils::StartsWithNoCase(item->GetPath(), "addons://more/"))
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems->IsPath(CUtil::MusicPlaylistsLocation()) ||
                       m_vecItems->IsPath("special://musicplaylists/");

    CMusicDatabaseDirectory dir;
    // enable music info button on an album or on a song.
    if (item->IsAudio() && !item->IsPlayList() && !item->IsSmartPlayList() &&
        !item->m_bIsFolder)
    {
      buttons.Add(CONTEXT_BUTTON_SONG_INFO, 658);
    }
    else if (item->IsVideoDb())
    {
      if (!item->m_bIsFolder) // music video
       buttons.Add(CONTEXT_BUTTON_INFO, 20393);
      if (StringUtils::StartsWithNoCase(item->GetPath(), "videodb://musicvideos/artists/") && item->m_bIsFolder)
      {
        long idArtist = m_musicdatabase.GetArtistByName(m_vecItems->Get(itemNumber)->GetLabel());
        if (idArtist > - 1)
          buttons.Add(CONTEXT_BUTTON_INFO,21891);
      }
    }
    else if (!inPlaylists && (dir.HasAlbumInfo(item->GetPath())||
                              dir.IsArtistDir(item->GetPath())   )      &&
             !dir.IsAllItem(item->GetPath()) && !item->IsParentFolder() &&
             !item->IsPlugin() && !item->IsScript() &&
             !StringUtils::StartsWithNoCase(item->GetPath(), "musicsearch://"))
    {
      if (dir.IsArtistDir(item->GetPath()))
        buttons.Add(CONTEXT_BUTTON_INFO, 21891);
      else
        buttons.Add(CONTEXT_BUTTON_INFO, 13351);
    }

    // enable query all albums button only in album view
    if (dir.HasAlbumInfo(item->GetPath()) && !dir.IsAllItem(item->GetPath()) &&
        item->m_bIsFolder && !item->IsVideoDb() && !item->IsParentFolder()   &&
       !item->IsPlugin() && !StringUtils::StartsWithNoCase(item->GetPath(), "musicsearch://"))
    {
      buttons.Add(CONTEXT_BUTTON_INFO_ALL, 20059);
    }

    // enable query all artist button only in album view
    if (dir.IsArtistDir(item->GetPath()) && !dir.IsAllItem(item->GetPath()) &&
      item->m_bIsFolder && !item->IsVideoDb())
    {
      ADDON::ScraperPtr info;
      m_musicdatabase.GetScraperForPath(item->GetPath(), info, ADDON::ADDON_SCRAPER_ARTISTS);
      if (info && info->Supports(CONTENT_ARTISTS))
        buttons.Add(CONTEXT_BUTTON_INFO_ALL, 21884);
    }

    //Set default or clear default
    NODE_TYPE nodetype = dir.GetDirectoryType(item->GetPath());
    if (!item->IsParentFolder() && !inPlaylists &&
        (nodetype == NODE_TYPE_ROOT     ||
         nodetype == NODE_TYPE_OVERVIEW ||
         nodetype == NODE_TYPE_TOP100))
    {
      if (!item->IsPath(CSettings::Get().GetString("mymusic.defaultlibview")))
        buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // set default
      if (!CSettings::Get().GetString("mymusic.defaultlibview").empty())
        buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // clear default
    }
    NODE_TYPE childtype = dir.GetDirectoryChildType(item->GetPath());
    if (childtype == NODE_TYPE_ALBUM               ||
        childtype == NODE_TYPE_ARTIST              ||
        nodetype == NODE_TYPE_GENRE                ||
        nodetype == NODE_TYPE_ALBUM                ||
        nodetype == NODE_TYPE_ALBUM_RECENTLY_ADDED ||
        nodetype == NODE_TYPE_ALBUM_COMPILATIONS)
    {
      // we allow the user to set content for
      // 1. general artist and album nodes
      // 2. specific per genre
      // 3. specific per artist
      // 4. specific per album
      buttons.Add(CONTEXT_BUTTON_SET_CONTENT,20195);
    }
    if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetArtist().size() > 0)
    {
      CVideoDatabase database;
      database.Open();
      if (database.GetMatchingMusicVideo(StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator)) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20400);
    }
    if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetArtist().size() > 0 &&
        item->GetMusicInfoTag()->GetAlbum().size() > 0 &&
        item->GetMusicInfoTag()->GetTitle().size() > 0)
    {
      CVideoDatabase database;
      database.Open();
      if (database.GetMatchingMusicVideo(StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator),item->GetMusicInfoTag()->GetAlbum(),item->GetMusicInfoTag()->GetTitle()) > -1)
        buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20401);
    }
    if (item->HasVideoInfoTag() && !item->m_bIsFolder)
    {
      if (item->GetVideoInfoTag()->m_playCount > 0)
        buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
      else
        buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
      if ((CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) && !item->IsPlugin())
      {
        buttons.Add(CONTEXT_BUTTON_RENAME, 16105);
        buttons.Add(CONTEXT_BUTTON_DELETE, 646);
      }
    }
    if (inPlaylists && URIUtils::GetFileName(item->GetPath()) != "PartyMode.xsp"
                    && (item->IsPlayList() || item->IsSmartPlayList()))
      buttons.Add(CONTEXT_BUTTON_DELETE, 117);

    if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
      buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
  }
  // noncontextual buttons

  CGUIWindowMusicBase::GetNonContextButtons(buttons);
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
      if (!item->IsVideoDb())
        return CGUIWindowMusicBase::OnContextButton(itemNumber,button);

      // music videos - artists
      if (StringUtils::StartsWithNoCase(item->GetPath(), "videodb://musicvideos/artists/"))
      {
        long idArtist = m_musicdatabase.GetArtistByName(item->GetLabel());
        if (idArtist == -1)
          return false;
        std::string path = StringUtils::Format("musicdb://artists/%ld/", idArtist);
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
        long idAlbum = m_musicdatabase.GetAlbumByName(item->GetLabel());
        if (idAlbum == -1)
          return false;
        std::string path = StringUtils::Format("musicdb://albums/%ld/", idAlbum);
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
        CGUIWindowVideoNav* pWindow = (CGUIWindowVideoNav*)g_windowManager.GetWindow(WINDOW_VIDEO_NAV);
        if (pWindow)
        {
          ADDON::ScraperPtr info;
          pWindow->OnInfo(item.get(),info);
          Refresh();
        }
      }
      return true;
    }

  case CONTEXT_BUTTON_INFO_ALL:
    OnInfoAll(itemNumber);
    return true;

  case CONTEXT_BUTTON_SET_DEFAULT:
    CSettings::Get().SetString("mymusic.defaultlibview", GetQuickpathName(item->GetPath()));
    CSettings::Get().Save();
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    CSettings::Get().SetString("mymusic.defaultlibview", "");
    CSettings::Get().Save();
    return true;

  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      std::string strPath;
      CVideoDatabase database;
      database.Open();
      strPath = StringUtils::Format("videodb://musicvideos/artists/%i/",
                                    database.GetMatchingMusicVideo(StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator)));
      g_windowManager.ActivateWindow(WINDOW_VIDEO_NAV,strPath);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CVideoDatabase database;
      database.Open();
      CVideoInfoTag details;
      database.GetMusicVideoInfo("",details,database.GetMatchingMusicVideo(StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator),item->GetMusicInfoTag()->GetAlbum(),item->GetMusicInfoTag()->GetTitle()));
      CApplicationMessenger::Get().PlayFile(CFileItem(details));
      return true;
    }

  case CONTEXT_BUTTON_RENAME:
    CGUIDialogVideoInfo::UpdateVideoItemTitle(item);
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Refresh();
    return true;

  case CONTEXT_BUTTON_DELETE:
    if (item->IsPlayList() || item->IsSmartPlayList())
    {
      item->m_bIsFolder = false;
      CFileUtils::DeleteItem(item);
    }
    else
    {
      CGUIDialogVideoInfo::DeleteVideoItemFromDatabase(item);
      CUtil::DeleteVideoDatabaseDirectoryCache();
    }
    Refresh();
    return true;

  case CONTEXT_BUTTON_SET_CONTENT:
    {
      ADDON::ScraperPtr scraper;
      std::string path(item->GetPath());
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(item->GetPath(), params);
      CONTENT_TYPE content = CONTENT_ALBUMS;
      if (params.GetAlbumId() != -1)
        path = StringUtils::Format("musicdb://albums/%li/",params.GetAlbumId());
      else if (params.GetArtistId() != -1)
      {
        path = StringUtils::Format("musicdb://artists/%li/",params.GetArtistId());
        content = CONTENT_ARTISTS;
      }

      if (m_vecItems->IsPath("musicdb://genres/") || item->IsPath("musicdb://artists/"))
      {
        content = CONTENT_ARTISTS;
      }

      if (!m_musicdatabase.GetScraperForPath(path, scraper, ADDON::ScraperTypeFromContent(content)))
      {
        ADDON::AddonPtr defaultScraper;
        if (ADDON::CAddonMgr::Get().GetDefault(ADDON::ScraperTypeFromContent(content), defaultScraper))
        {
          scraper = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper->Clone());
        }
      }

      if (CGUIDialogContentSettings::Show(scraper, content))
      {
        m_musicdatabase.SetScraperForPath(path,scraper);
        if (CGUIDialogYesNo::ShowAndGetInput(20442,20443,20444,20022))
        {
          OnInfoAll(itemNumber,true,true);
        }

      }
      return true;
    }

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
  CLog::Log(LOGDEBUG,"CGUIWindowMusicNav, opening playlist [%s]", strPlayList.c_str());

  unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return false; //hmmm unable to load playlist?
    }
    CPlayList playlist = *pPlayList;
    // convert playlist items to songs
    for (int i = 0; i < (int)playlist.size(); ++i)
    {
      items.Add(playlist[i]);
    }
  }

  return true;
}

void CGUIWindowMusicNav::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
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
  if (m_bDisplayEmptyDatabaseMessage)
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746));
  else
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"");
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
      // add earch share
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
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "genres")
    return "musicdb://genres/";
  else if (lower == "artists")
    return "musicdb://artists/";
  else if (lower == "albums")
    return "musicdb://albums/";
  else if (lower == "singles")
    return "musicdb://singles/";
  else if (lower == "songs")
    return "musicdb://songs/";
  else if (lower == "top100")
    return "musicdb://top100/";
  else if (lower == "top100songs")
    return "musicdb://top100/songs/";
  else if (lower == "top100albums")
    return "musicdb://top100/albums/";
  else if (lower == "recentlyaddedalbums")
    return "musicdb://recentlyaddedalbums/";
  else if (lower == "recentlyplayedalbums")
   return "musicdb://recentlyplayedalbums/";
  else if (lower == "compilations")
    return "musicdb://compilations/";
  else if (lower == "years")
    return "musicdb://years/";
  return CGUIWindowMusicBase::GetStartFolder(dir);
}
