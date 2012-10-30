/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/SystemClock.h"
#include "system.h"
#include "GUIUserMessages.h"
#include "GUIWindowMusicBase.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "filesystem/ZipManager.h"
#ifdef HAS_FILESYSTEM_DAAP
#include "filesystem/DAAPDirectory.h"
#endif
#include "playlists/PlayListFactory.h"
#include "Util.h"
#include "playlists/PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "filesystem/DirectoryCache.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif
#include "GUIPassword.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "PartyModeManager.h"
#include "GUIInfoManager.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "addons/GUIDialogAddonInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "music/LastFmManager.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "storage/MediaManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "music/infoscanner/MusicInfoScanner.h"

using namespace std;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace PLAYLIST;
using namespace MUSIC_GRABBER;
using namespace MUSIC_INFO;

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNTYPE         5

CGUIWindowMusicBase::CGUIWindowMusicBase(int id, const CStdString &xmlFile)
    : CGUIMediaWindow(id, xmlFile)
{

}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{
}

bool CGUIWindowMusicBase::OnBack(int actionID)
{
  if (!g_application.IsMusicScanning())
  {
    CUtil::RemoveTempFiles();
  }
  return CGUIMediaWindow::OnBack(actionID);
}

/*!
 \brief Handle messages on window.
 \param message GUI Message that can be reacted on.
 \return if a message can't be processed, return \e false

 On these messages this class reacts.\n
 When retrieving...
  - #GUI_MSG_WINDOW_DEINIT\n
   ...the last focused control is saved to m_iLastControl.
  - #GUI_MSG_WINDOW_INIT\n
   ...the musicdatabase is opend and the music extensions and shares are set.
   The last focused control is set.
  - #GUI_MSG_CLICKED\n
   ... the base class reacts on the following controls:\n
    Buttons:\n
    - #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
    - #CONTROL_BTNTYPE - switch between music windows
    - #CONTROL_BTNSEARCH - Search for items\n
    Other Controls:
    - The container controls\n
     Have the following actions in message them clicking on them.
     - #ACTION_QUEUE_ITEM - add selected item to playlist
     - #ACTION_SHOW_INFO - retrieve album info from the internet
     - #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
 */
bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_musicdatabase.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_musicdatabase.Open();

      if (!CGUIMediaWindow::OnMessage(message))
        return false;

      // save current window, unless the current window is the music playlist window
      if (GetID() != WINDOW_MUSIC_PLAYLIST &&
          g_settings.m_iMyMusicStartWindow != GetID())
      {
        g_settings.m_iMyMusicStartWindow = GetID();
        g_settings.Save();
      }

      return true;
    }
    break;

  // update the display
  case GUI_MSG_SCAN_FINISHED:
  case GUI_MSG_REFRESH_THUMBS:
    Refresh();
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        g_windowManager.SendMessage(msg);

        int nWindow = WINDOW_MUSIC_FILES + msg.GetParam1();

        if (nWindow == GetID())
          return true;

        g_settings.m_iMyMusicStartWindow = nWindow;
        g_settings.Save();
        g_windowManager.ChangeActiveWindow(nWindow);

        CGUIMessage msg2(GUI_MSG_SETFOCUS, g_settings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
        g_windowManager.SendMessage(msg2);

        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the playlists directory
          if (m_vecItems->GetPath().Equals("special://musicplaylists/"))
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_MUSIC_FILES &&
                   g_guiSettings.GetBool("filelists.allowfiledeletion"))
          {
            OnDeleteItem(iItem);
          }

          else
            return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused())
              return false;
            if (g_application.GetPlaySpeed() != 1)
              return false;
          }

          // not playing audio, or playback speed == 1
          PlayItem(iItem);

          return true;
        }
      }
    }
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowMusicBase::OnInfoAll(int iItem, bool bCurrent, bool refresh)
{
  CMusicDatabaseDirectory dir;
  CStdString strPath = m_vecItems->GetPath();
  if (bCurrent)
    strPath = m_vecItems->Get(iItem)->GetPath();

  if (dir.HasAlbumInfo(strPath) ||
      CMusicDatabaseDirectory::GetDirectoryChildType(strPath) == 
      MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM)
    g_application.StartMusicAlbumScan(strPath,refresh);
  else
    g_application.StartMusicArtistScan(strPath,refresh);
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIDialogMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem, bool bShowInfo)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() )
    return;

  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->IsVideoDb())
  { // music video
    OnContextButton(iItem, CONTEXT_BUTTON_INFO);
    return;
  }

  if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
  {
    CGUIDialogAddonInfo::ShowForItem(item);
    return;
  }

  OnInfo(item.get(), bShowInfo);
}

void CGUIWindowMusicBase::OnInfo(CFileItem *pItem, bool bShowInfo)
{
  if ((pItem->IsMusicDb() && !pItem->HasMusicInfoTag()) || pItem->IsParentFolder() ||
       URIUtils::IsSpecial(pItem->GetPath()) || pItem->GetPath().Left(14).Equals("musicsearch://"))
    return; // nothing to do

  if (!pItem->m_bIsFolder)
  { // song lookup
    ShowSongInfo(pItem);
    return;
  }

  CStdString strPath = pItem->GetPath();

  // Try to find an album to lookup from the current item
  CAlbum album;
  CArtist artist;
  bool foundAlbum = false;

  album.idAlbum = -1;

  // we have a folder
  if (pItem->IsMusicDb())
  {
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(pItem->GetPath(), params);
    if (params.GetAlbumId() == -1)
    { // artist lookup
      artist.idArtist = params.GetArtistId();
      artist.strArtist = StringUtils::Join(pItem->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    }
    else
    { // album lookup
      album.idAlbum = params.GetAlbumId();
      album.strAlbum = pItem->GetMusicInfoTag()->GetAlbum();
      album.artist = pItem->GetMusicInfoTag()->GetArtist();

      // we're going to need it's path as well (we assume that there's only one) - this is for
      // assigning thumbs to folders, and obtaining the local folder.jpg
      m_musicdatabase.GetAlbumPath(album.idAlbum, strPath);
    }
  }
  else
  { // from filemode, so find the albums in the folder
    CFileItemList items;
    GetDirectory(strPath, items);

    // show dialog box indicating we're searching the album name
    if (m_dlgProgress && bShowInfo)
    {
      m_dlgProgress->SetHeading(185);
      m_dlgProgress->SetLine(0, 501);
      m_dlgProgress->SetLine(1, "");
      m_dlgProgress->SetLine(2, "");
      m_dlgProgress->StartModal();
      m_dlgProgress->Progress();
      if (m_dlgProgress->IsCanceled())
        return;
    }
    // check the first song we find in the folder, and grab it's album info
    for (int i = 0; i < items.Size() && !foundAlbum; i++)
    {
      CFileItemPtr pItem = items[i];
      pItem->LoadMusicTag();
      if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded() &&
         !pItem->GetMusicInfoTag()->GetAlbum().IsEmpty())
      {
        // great, have a song - use it.
        CSong song(*pItem->GetMusicInfoTag());
        // this function won't be needed if/when the tag has idSong information
        if (!m_musicdatabase.GetAlbumFromSong(song, album))
        { // album isn't in the database - construct it from the tag info we have
          CMusicInfoTag *tag = pItem->GetMusicInfoTag();
          album.strAlbum = tag->GetAlbum();
          album.artist = tag->GetAlbumArtist().empty() ? tag->GetArtist() : tag->GetAlbumArtist();
          album.idAlbum = -1; // the -1 indicates it's not in the database
        }
        foundAlbum = true;
      }
    }
    if (!foundAlbum)
    {
      CLog::Log(LOGINFO, "%s called on a folder containing no songs with tag info - nothing can be done", __FUNCTION__);
      if (m_dlgProgress && bShowInfo)
        m_dlgProgress->Close();
      return;
    }

    if (m_dlgProgress && bShowInfo)
      m_dlgProgress->Close();
  }

  if (album.idAlbum == -1 && foundAlbum == false)
    ShowArtistInfo(artist, pItem->GetPath(), false, bShowInfo);
  else
    ShowAlbumInfo(album, strPath, false, bShowInfo);
}

void CGUIWindowMusicBase::OnManualAlbumInfo()
{
  CAlbum album;
  album.idAlbum = -1; // not in the db
  if (!CGUIKeyboardFactory::ShowAndGetInput(album.strAlbum, g_localizeStrings.Get(16011), false))
    return;

  CStdString strArtist = StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator);
  if (!CGUIKeyboardFactory::ShowAndGetInput(strArtist, g_localizeStrings.Get(16025), false))
    return;

  ShowAlbumInfo(album,"",true);
}

void CGUIWindowMusicBase::ShowArtistInfo(const CArtist& artist, const CStdString& path, bool bRefresh, bool bShowInfo)
{
  bool saveDb = artist.idArtist != -1;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    saveDb = false;

  // check cache
  CArtist artistInfo;
  if (!bRefresh && m_musicdatabase.GetArtistInfo(artist.idArtist, artistInfo))
  {
    if (!bShowInfo)
      return;

    CGUIDialogMusicInfo *pDlgArtistInfo = (CGUIDialogMusicInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
    if (pDlgArtistInfo)
    {
      pDlgArtistInfo->SetArtist(artistInfo, path);

      if (bShowInfo)
        pDlgArtistInfo->DoModal();

      if (!pDlgArtistInfo->NeedRefresh())
      {
        if (pDlgArtistInfo->HasUpdatedThumb())
          Refresh();

        return;
      }
      bRefresh = true;
      m_musicdatabase.DeleteArtistInfo(artistInfo.idArtist);
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsMusicScanning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CMusicArtistInfo info;
  if (FindArtistInfo(artist.strArtist, info, bShowInfo ? (bRefresh ? SELECTION_FORCED : SELECTION_ALLOWED) : SELECTION_AUTO))
  {
    // download the album info
    if ( info.Loaded() )
    {
      if (saveDb)
      {
        // save to database
        m_musicdatabase.SetArtistInfo(artist.idArtist, info.GetArtist());
      }
      if (m_dlgProgress && bShowInfo)
        m_dlgProgress->Close();

      // ok, show album info
      CGUIDialogMusicInfo *pDlgArtistInfo = (CGUIDialogMusicInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
      if (pDlgArtistInfo)
      {
        pDlgArtistInfo->SetArtist(info.GetArtist(), path);
        if (bShowInfo)
          pDlgArtistInfo->DoModal();

        CArtist artistInfo = info.GetArtist();
        artistInfo.idArtist = artist.idArtist;
/*
        if (pDlgAlbumInfo->HasUpdatedThumb())
          UpdateThumb(artistInfo, path);
*/
        // just update for now
        Refresh();
        if (pDlgArtistInfo->NeedRefresh())
        {
          m_musicdatabase.DeleteArtistInfo(artistInfo.idArtist);
          ShowArtistInfo(artist, path, true, bShowInfo);
          return;
        }
      }
    }
    else
    {
      // failed 2 download album info
      CGUIDialogOK::ShowAndGetInput(21889, 0, 20199, 0);
    }
  }

  if (m_dlgProgress && bShowInfo)
    m_dlgProgress->Close();
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CAlbum& album, const CStdString& path, bool bRefresh, bool bShowInfo)
{
  bool saveDb = album.idAlbum != -1;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    saveDb = false;

  // check cache
  CAlbum albumInfo;
  if (!bRefresh && m_musicdatabase.GetAlbumInfo(album.idAlbum, albumInfo, &albumInfo.songs))
  {
    if (!bShowInfo)
      return;

    CGUIDialogMusicInfo *pDlgAlbumInfo = (CGUIDialogMusicInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
      pDlgAlbumInfo->SetAlbum(albumInfo, path);
      if (bShowInfo)
        pDlgAlbumInfo->DoModal();

      if (!pDlgAlbumInfo->NeedRefresh())
      {
        if (pDlgAlbumInfo->HasUpdatedThumb())
          UpdateThumb(albumInfo, path);
        return;
      }
      bRefresh = true;
      m_musicdatabase.DeleteAlbumInfo(albumInfo.idAlbum);
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsMusicScanning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CMusicAlbumInfo info;
  if (FindAlbumInfo(album.strAlbum, StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator), info, bShowInfo ? (bRefresh ? SELECTION_FORCED : SELECTION_ALLOWED) : SELECTION_AUTO))
  {
    // download the album info
    if ( info.Loaded() )
    {
      // set album title from musicinfotag, not the one we got from allmusic.com
      info.SetTitle(album.strAlbum);

      if (saveDb)
      {
        // save to database
        m_musicdatabase.SetAlbumInfo(album.idAlbum, info.GetAlbum(), info.GetSongs());
      }
      if (m_dlgProgress && bShowInfo)
        m_dlgProgress->Close();

      UpdateThumb(album, path);

      // ok, show album info
      CGUIDialogMusicInfo *pDlgAlbumInfo = (CGUIDialogMusicInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
      if (pDlgAlbumInfo)
      {
        pDlgAlbumInfo->SetAlbum(info.GetAlbum(), path);
        if (bShowInfo)
          pDlgAlbumInfo->DoModal();

        CAlbum albumInfo = info.GetAlbum();
        albumInfo.idAlbum = album.idAlbum;
        if (pDlgAlbumInfo->HasUpdatedThumb())
          UpdateThumb(albumInfo, path);

        if (pDlgAlbumInfo->NeedRefresh())
        {
          m_musicdatabase.DeleteAlbumInfo(albumInfo.idAlbum);
          ShowAlbumInfo(album, path, true, bShowInfo);
          return;
        }
      }
    }
    else
    {
      // failed 2 download album info
      CGUIDialogOK::ShowAndGetInput(185, 0, 500, 0);
    }
  }

  if (m_dlgProgress && bShowInfo)
    m_dlgProgress->Close();
}

void CGUIWindowMusicBase::ShowSongInfo(CFileItem* pItem)
{
  CGUIDialogSongInfo *dialog = (CGUIDialogSongInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_SONG_INFO);
  if (dialog)
  {
    if (!pItem->IsMusicDb())
      pItem->LoadMusicTag();
    if (!pItem->HasMusicInfoTag())
      return;

    dialog->SetSong(pItem);
    dialog->DoModal(GetID());
    if (dialog->NeedsUpdate())
      Refresh(true); // update our file list
  }
}

/*
/// \brief Can be overwritten to implement an own tag filling function.
/// \param items File items to fill
void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{
}
*/

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowMusicBase::RetrieveMusicInfo()
{
  unsigned int startTick = XbmcThreads::SystemClockMillis();

  OnRetrieveMusicInfo(*m_vecItems);

  CLog::Log(LOGDEBUG, "RetrieveMusicInfo() took %u msec",
            XbmcThreads::SystemClockMillis() - startTick);
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
  // don't re-queue items from playlist window
  if ( iItem < 0 || iItem >= m_vecItems->Size() || GetID() == WINDOW_MUSIC_PLAYLIST) return ;

  int iOldSize=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size();

  // add item 2 playlist (make a copy as we alter the queuing state)
  CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

  if (item->IsRAR() || item->IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item->CanQueue())
    item->SetCanQueue(true);

  CLog::Log(LOGDEBUG, "Adding file %s%s to music playlist", item->GetPath().c_str(), item->m_bIsFolder ? " (folder) " : "");
  CFileItemList queuedItems;
  AddItemToPlayList(item, queuedItems);

  // select next item
  m_viewControl.SetSelectedItem(iItem + 1);

  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled())
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio())
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistmusic://");

    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play(iOldSize); // start playing at the first new item
  }
}

/// \brief Add unique file and folders and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  // fast lookup is needed here
  queuedItems.SetFastLookup(true);

  if (pItem->IsMusicDb() && pItem->m_bIsFolder && !pItem->IsParentFolder())
  { // we have a music database folder, just grab the "all" item underneath it
    CMusicDatabaseDirectory dir;
    if (!dir.ContainsSongs(pItem->GetPath()))
    { // grab the ALL item in this category
      // Genres will still require 2 lookups, and queuing the entire Genre folder
      // will require 3 lookups (genre, artist, album)
      CMusicDbUrl musicUrl;
      musicUrl.FromString(pItem->GetPath());
      musicUrl.AppendPath("-1/");
      CFileItemPtr item(new CFileItem(musicUrl.ToString(), true));
      item->SetCanQueue(true); // workaround for CanQueue() check above
      AddItemToPlayList(item, queuedItems);
      return;
    }
  }
  if (pItem->m_bIsFolder || (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
  {
    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "music" ) )
        return ;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->GetPath(), items);
    //OnRetrieveMusicInfo(items);
    FormatAndSort(items);
    for (int i = 0; i < items.Size(); ++i)
      AddItemToPlayList(items[i], queuedItems);
  }
  else
  {
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if (pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->GetPath()))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
        {
          AddItemToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsPlugin() && pItem->GetProperty("isplayable") == "true")
    {
      // python files can be played
      queuedItems.Add(pItem);
    }
    else if (!pItem->IsNFO() && pItem->IsAudio())
    {
      CFileItemPtr itemCheck = queuedItems.Get(pItem->GetPath());
      if (!itemCheck || itemCheck->m_lStartOffset != pItem->m_lStartOffset)
      { // add item
        CFileItemPtr item(new CFileItem(*pItem));
        m_musicdatabase.SetPropertiesForFileItem(*item);
        queuedItems.Add(item);
      }
    }
  }
}

void CGUIWindowMusicBase::UpdateButtons()
{
  // Update window selection control

  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_windowManager.SendMessage(msg);

  // Add labels to the window selection
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(g_localizeStrings.Get(744)); // Files
  g_windowManager.SendMessage(msg2);

  msg2.SetLabel(g_localizeStrings.Get(15100)); // Library
  g_windowManager.SendMessage(msg2);

  // Select the current window as default item
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_settings.m_iMyMusicStartWindow - WINDOW_MUSIC_FILES);

  CGUIMediaWindow::UpdateButtons();
}

bool CGUIWindowMusicBase::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album, ALLOW_SELECTION allowSelection)
{
  // show dialog box indicating we're searching the album
  if (m_dlgProgress && allowSelection != SELECTION_AUTO)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, strAlbum);
    m_dlgProgress->SetLine(1, strArtist);
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
  }

  CMusicInfoScanner scanner;
  CStdString strPath;
  CStdString strTempAlbum(strAlbum);
  CStdString strTempArtist(strArtist);
  long idAlbum = m_musicdatabase.GetAlbumByName(strAlbum,strArtist);
  strPath.Format("musicdb://3/%d/",idAlbum);

  bool bCanceled(false);
  bool needsRefresh(true);
  do
  {
    if (!scanner.DownloadAlbumInfo(strPath,strTempArtist,strTempAlbum,bCanceled,album,m_dlgProgress))
    {
      if (bCanceled)
        return false;
      if (m_dlgProgress && allowSelection != SELECTION_AUTO)
      {
        if (!CGUIKeyboardFactory::ShowAndGetInput(strTempAlbum, g_localizeStrings.Get(16011), false))
          return false;

        if (!CGUIKeyboardFactory::ShowAndGetInput(strTempArtist, g_localizeStrings.Get(16025), false))
          return false;
      }
      else
        needsRefresh = false;
    }
    else
      needsRefresh = false;
  }
  while (needsRefresh || bCanceled);

  // Read the album information from the database if we are dealing with a DB album.
  if (idAlbum != -1)
    m_musicdatabase.GetAlbumInfo(idAlbum,album.GetAlbum(),&album.GetAlbum().songs);

  album.SetLoaded(true);
  return true;
}

bool CGUIWindowMusicBase::FindArtistInfo(const CStdString& strArtist, CMusicArtistInfo& artist, ALLOW_SELECTION allowSelection)
{
  // show dialog box indicating we're searching the album
  if (m_dlgProgress && allowSelection != SELECTION_AUTO)
  {
    m_dlgProgress->SetHeading(21889);
    m_dlgProgress->SetLine(0, strArtist);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
  }

  CMusicInfoScanner scanner;
  CStdString strPath;
  CStdString strTempArtist(strArtist);
  long idArtist = m_musicdatabase.GetArtistByName(strArtist);
  strPath.Format("musicdb://2/%u/",idArtist);

  bool bCanceled(false);
  bool needsRefresh(true);
  do
  {
    if (!scanner.DownloadArtistInfo(strPath,strTempArtist,bCanceled,m_dlgProgress))
    {
      if (bCanceled)
        return false;
      if (m_dlgProgress && allowSelection != SELECTION_AUTO)
      {
        if (!CGUIKeyboardFactory::ShowAndGetInput(strTempArtist, g_localizeStrings.Get(16025), false))
          return false;
      }
      else
        needsRefresh = false;
    }
    else
      needsRefresh = false;
  }
  while (needsRefresh || bCanceled);

  if (!m_musicdatabase.GetArtistInfo(idArtist,artist.GetArtist()))
    return false;

  artist.SetLoaded();
  return true;
}

void CGUIWindowMusicBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
  {
    if (item && !item->IsParentFolder())
    {
      if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
        buttons.Add(CONTEXT_BUTTON_INFO,24003); // Add-on info
      if (item->GetExtraInfo().Equals("lastfmloved"))
      {
        buttons.Add(CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM, 15295); //unlove
      }
      else if (item->GetExtraInfo().Equals("lastfmbanned"))
      {
        buttons.Add(CONTEXT_BUTTON_LASTFM_UNBAN_ITEM, 15296); //unban
      }
      else if (item->CanQueue() && !item->IsAddonsPath() && !item->IsScript())
      {
        buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347); //queue

        // allow a folder to be ad-hoc queued and played by the default player
        if (item->m_bIsFolder || (item->IsPlayList() &&
           !g_advancedSettings.m_playlistAsFolders))
        {
          buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
        }
        else
        { // check what players we have, if we have multiple display play with option
          VECPLAYERCORES vecCores;
          CPlayerCoreFactory::GetPlayers(*item, vecCores);
          if (vecCores.size() >= 1)
            buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...
        }
        if (item->IsSmartPlayList())
        {
            buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
        }

        if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
          buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
        else if (item->IsPlayList() || m_vecItems->IsPlayList())
          buttons.Add(CONTEXT_BUTTON_EDIT, 586);
      }
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowMusicBase::GetNonContextButtons(CContextButtons &buttons)
{
  if (!m_vecItems->IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);
}

bool CGUIWindowMusicBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  switch (button)
  {
  case CONTEXT_BUTTON_QUEUE_ITEM:
    OnQueueItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_INFO:
    OnInfo(itemNumber);
    return true;

  case CONTEXT_BUTTON_SONG_INFO:
    {
      ShowSongInfo(item.get());
      return true;
    }

  case CONTEXT_BUTTON_EDIT:
    {
      CStdString playlist = item->IsPlayList() ? item->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR, playlist);
      // need to update
      m_vecItems->RemoveDiscCache(GetID());
      return true;
    }

  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      CStdString playlist = item->IsSmartPlayList() ? item->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "music"))
        Refresh(true); // need to update
      return true;
    }

  case CONTEXT_BUTTON_PLAY_ITEM:
    PlayItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_WITH:
    {
      VECPLAYERCORES vecCores;  // base class?
      CPlayerCoreFactory::GetPlayers(*item, vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
      if( g_application.m_eForcedNextPlayer != EPC_NONE )
        OnClick(itemNumber);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_MUSIC, item->GetPath());
    return true;

  case CONTEXT_BUTTON_STOP_SCANNING:
    {
      g_application.StopMusicScan();
      return true;
    }

  case CONTEXT_BUTTON_NOW_PLAYING:
    g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;

  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;

  case CONTEXT_BUTTON_SETTINGS:
    g_windowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
    return true;
  case CONTEXT_BUTTON_LASTFM_UNBAN_ITEM:
    if (CLastFmManager::GetInstance()->Unban(*item->GetMusicInfoTag()))
    {
      g_directoryCache.ClearDirectory(m_vecItems->GetPath());
      Refresh(true);
    }
    return true;
  case CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM:
    if (CLastFmManager::GetInstance()->Unlove(*item->GetMusicInfoTag()))
    {
      g_directoryCache.ClearDirectory(m_vecItems->GetPath());
      Refresh(true);
    }
    return true;
  default:
    break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowMusicBase::OnRipCD()
{
  if(g_mediaManager.IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CCDDARipper::GetInstance().RipCD();
#endif
    }
    else
      CGUIDialogOK::ShowAndGetInput(257, 20099, 0, 0);
  }
}

void CGUIWindowMusicBase::OnRipTrack(int iItem)
{
  if(g_mediaManager.IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CFileItemPtr item = m_vecItems->Get(iItem);
      CCDDARipper::GetInstance().RipTrack(item.get());
#endif
    }
    else
      CGUIDialogOK::ShowAndGetInput(257, 20099, 0, 0);
  }
}

void CGUIWindowMusicBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the current playlist

  const CFileItemPtr pItem = m_vecItems->Get(iItem);

  // special case for DAAP playlist folders
  bool bIsDAAPplaylist = false;
#ifdef HAS_FILESYSTEM_DAAP
  if (pItem->IsDAAP() && pItem->m_bIsFolder)
  {
    CDAAPDirectory dirDAAP;
    if (dirDAAP.GetCurrLevel(pItem->GetPath()) == 0)
      bIsDAAPplaylist = true;
  }
#endif
  // if its a folder, build a playlist
  if ((pItem->m_bIsFolder && !pItem->IsPlugin()) || (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
  {
    // make a copy so that we can alter the queue state
    CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder())
      return;

    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);
    if (g_partyModeManager.IsEnabled())
    {
      g_partyModeManager.AddUserSongs(queuedItems, true);
      return;
    }

    /*
    CStdString strPlayListDirectory = m_vecItems->GetPath();
    URIUtils::RemoveSlashAtEnd(strPlayListDirectory);
    */

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);

    // activate the playlist window if its not activated yet
    if (bIsDAAPplaylist && GetID() == g_windowManager.GetActiveWindow())
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

    // play!
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->GetPath());
  }
  else
  {
    // just a single item, play it
    // TODO: Add music-specific code for single playback of an item here (See OnClick in MediaWindow, and OnPlayMedia below)
    OnClick(iItem);
  }
}

void CGUIWindowMusicBase::LoadPlayList(const CStdString& strPlayList)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if (pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistmusic://");
    // activate the playlist window if its not activated yet
    if (GetID() == g_windowManager.GetActiveWindow() && iSize > 1)
    {
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
  }
}

bool CGUIWindowMusicBase::OnPlayMedia(int iItem)
{
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  // party mode
  if (g_partyModeManager.IsEnabled() && !pItem->IsLastFM())
  {
    CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }
  else if (!pItem->IsPlayList() && !pItem->IsInternetStream())
  { // single music file - if we get here then we have autoplaynextitem turned off or queuebydefault
    // turned on, but we still want to use the playlist player in order to handle more queued items
    // following etc.
    // Karaoke items also can be added in runtime (while the song is played), so it should be queued too.
    if ( (g_guiSettings.GetBool("musicplayer.queuebydefault") && g_windowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST_EDITOR)
       || pItem->IsKaraoke() )
    {
      // TODO: Should the playlist be cleared if nothing is already playing?
      OnQueueItem(iItem);
      return true;
    }
    g_playlistPlayer.Reset();
    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Add(PLAYLIST_MUSIC, pItem);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play();
    return true;
  }
  return CGUIMediaWindow::OnPlayMedia(iItem);
}

void CGUIWindowMusicBase::UpdateThumb(const CAlbum &album, const CStdString &path)
{
  // check user permissions
  bool saveDb = album.idAlbum != -1;
  bool saveDirThumb = true;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
  {
    saveDb = false;
    saveDirThumb = false;
  }

  CStdString albumThumb = m_musicdatabase.GetArtForItem(album.idAlbum, "album", "thumb");

  // Update the thumb in the music database (songs + albums)
  CStdString albumPath(path);
  if (saveDb && CFile::Exists(albumThumb))
    m_musicdatabase.SaveAlbumThumb(album.idAlbum, albumThumb);

  // Update currently playing song if it's from the same album.  This is necessary as when the album
  // first gets it's cover, the info manager's item doesn't have the updated information (so will be
  // sending a blank thumb to the skin.)
  if (g_application.IsPlayingAudio())
  {
    const CMusicInfoTag* tag=g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      // really, this may not be enough as it is to reliably update this item.  eg think of various artists albums
      // that aren't tagged as such (and aren't yet scanned).  But we probably can't do anything better than this
      // in that case
      if (album.strAlbum == tag->GetAlbum() && (album.artist == tag->GetAlbumArtist() ||
                                                album.artist == tag->GetArtist()))
      {
        g_infoManager.SetCurrentAlbumThumb(albumThumb);
      }
    }
  }

  // Save this thumb as the directory thumb if it's the only album in the folder (files view nicety)
  // We do this by grabbing all the songs in the folder, and checking to see whether they come
  // from the same album.
  if (saveDirThumb && CFile::Exists(albumThumb) && !albumPath.IsEmpty() && !URIUtils::IsCDDA(albumPath))
  {
    CFileItemList items;
    GetDirectory(albumPath, items);
    OnRetrieveMusicInfo(items);
    VECSONGS songs;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->Loaded())
      {
        CSong song(*item->GetMusicInfoTag());
        songs.push_back(song);
      }
    }
    VECALBUMS albums;
    CMusicInfoScanner::CategoriseAlbums(songs, albums);
    if (albums.size() == 1)
    { // set as folder thumb as well
      CThumbLoader::SetCachedImage(items, "thumb", albumPath);
    }
  }

  // update the file listing - we have to update the whole lot, as it's likely that
  // more than just our thumbnaias changed
  // TODO: Ideally this would only be done when needed - at the moment we appear to be
  //       doing this for every lookup, possibly twice (see ShowAlbumInfo)
  Refresh(true);

  //  Do we have to autoswitch to the thumb control?
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), *m_vecItems));
  UpdateButtons();
}

void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{
  if (items.GetFolderCount()==items.Size() || items.IsMusicDb() ||
     (!g_guiSettings.GetBool("musicfiles.usetags") && !items.IsCDDA()))
  {
    return;
  }
  // Start the music info loader thread
  m_musicInfoLoader.SetProgressCallback(m_dlgProgress);
  m_musicInfoLoader.Load(items);

  bool bShowProgress=!g_windowManager.HasModalDialog();
  bool bProgressVisible=false;

  unsigned int tick=XbmcThreads::SystemClockMillis();

  while (m_musicInfoLoader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      unsigned int elapsed=XbmcThreads::SystemClockMillis()-tick;

      if (!bProgressVisible && elapsed>1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.GetPath());
        CStdString strStrippedPath = url.GetWithoutUserDetails();
        m_dlgProgress->SetHeading(189);
        m_dlgProgress->SetLine(0, 505);
        m_dlgProgress->SetLine(1, "");
        m_dlgProgress->SetLine(2, strStrippedPath );
        m_dlgProgress->StartModal();
        m_dlgProgress->ShowProgressBar(true);
        bProgressVisible = true;
      }

      if (bProgressVisible && m_dlgProgress && !m_dlgProgress->IsCanceled())
      { // keep GUI alive
        m_dlgProgress->Progress();
      }
    } // if (bShowProgress)
    Sleep(1);
  } // while (m_musicInfoLoader.IsLoading())

  if (bProgressVisible && m_dlgProgress)
    m_dlgProgress->Close();
}

bool CGUIWindowMusicBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.SetArt("thumb", "");
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);
  if (bResult)
    CMusicThumbLoader::FillThumb(items);

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://musicplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem(g_settings.GetUserDataItem("PartyMode.xsp"),false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->SetSpecialSort(SortSpecialOnBottom);
    newPlaylist->SetCanQueue(false);
    items.Add(newPlaylist);

    newPlaylist.reset(new CFileItem("newsmartplaylist://music", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->SetSpecialSort(SortSpecialOnBottom);
    newPlaylist->SetCanQueue(false);
    items.Add(newPlaylist);
  }

  return bResult;
}

void CGUIWindowMusicBase::OnPrepareFileItems(CFileItemList &items)
{
}

bool CGUIWindowMusicBase::CheckFilterAdvanced(CFileItemList &items) const
{
  CStdString content = items.GetContent();
  if ((items.IsMusicDb() || CanContainFilter(m_strFilterPath)) &&
      (content.Equals("artists") || content.Equals("albums") || content.Equals("songs")))
    return true;

  return false;
}

bool CGUIWindowMusicBase::CanContainFilter(const CStdString &strDirectory) const
{
  return StringUtils::StartsWith(strDirectory, "musicdb://");
}

void CGUIWindowMusicBase::OnInitWindow()
{
  CGUIMediaWindow::OnInitWindow();
  if (g_settings.m_musicNeedsUpdate == 27 && !g_application.IsMusicScanning() &&
      g_infoManager.GetLibraryBool(LIBRARY_HAS_MUSIC))
  {
    // rescan of music library required
    if (CGUIDialogYesNo::ShowAndGetInput(799, 800, 801, -1))
    {
      g_application.StartMusicScan("", CMusicInfoScanner::SCAN_RESCAN);
      g_settings.m_musicNeedsUpdate = 0; // once is enough (user may interrupt, but that's up to them)
      g_settings.Save();
    }
  }
}

CStdString CGUIWindowMusicBase::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/audio/";
  else if (dir.Equals("$PLAYLISTS") || dir.Equals("Playlists"))
    return "special://musicplaylists/";
  return CGUIMediaWindow::GetStartFolder(dir);
}
