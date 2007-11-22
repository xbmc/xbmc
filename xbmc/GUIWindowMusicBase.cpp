/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowMusicBase.h"
#include "GUIWindowMusicInfo.h"
#include "FileSystem/ZipManager.h"
#ifdef HAS_FILESYSTEM_DAAP
#include "FileSystem/DAAPDirectory.h"
#endif
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "FileSystem/DirectoryCache.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif
#include "GUIPassword.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogMediaSource.h"
#include "PartyModeManager.h"
#include "utils/GUIInfoManager.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "GUIDialogSongInfo.h"
#include "GUIDialogSmartPlaylistEditor.h"
#include "LastFmManager.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace MUSIC_GRABBER;

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNTYPE         5
#define CONTROL_LIST            50
#define CONTROL_THUMBS          51
#define CONTROL_BIGLIST         52

CGUIWindowMusicBase::CGUIWindowMusicBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{

}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{
}

/// \brief Handle actions on window.
/// \param action Action that can be reacted on.
bool CGUIWindowMusicBase::OnAction(const CAction& action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && !musicScan->IsDialogRunning())
    {
      CUtil::ThumbCacheClear();
      CUtil::RemoveTempFiles();
    }
  }

  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;
  }

  return CGUIMediaWindow::OnAction(action);
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
    - #CONTROL_LIST and #CONTROL_THUMB\n
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
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_musicdatabase.Open();

      if (!CGUIMediaWindow::OnMessage(message))
        return false;

      // save current window, unless the current window is the music playlist window
      if (GetID() != WINDOW_MUSIC_PLAYLIST && (DWORD) g_stSettings.m_iMyMusicStartWindow != GetID())
      {
        g_stSettings.m_iMyMusicStartWindow = GetID();
        g_settings.Save();
      }

      return true;
    }
    break;

    // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS:
    {
      Update(m_vecItems.m_strPath);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

        DWORD nWindow = WINDOW_MUSIC_FILES + msg.GetParam1();

        if (nWindow == GetID())
          return true;

        g_stSettings.m_iMyMusicStartWindow = nWindow;
        g_settings.Save();
        m_gWindowManager.ChangeActiveWindow(nWindow);

        CGUIMessage msg2(GUI_MSG_SETFOCUS, g_stSettings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
        g_graphicsContext.SendMessage(msg2);

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
          if (m_vecItems.m_strPath.Equals("special://musicplaylists/"))
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_MUSIC_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);

          else
            return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
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

void CGUIWindowMusicBase::OnInfoAll(int iItem)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(20097);
    m_dlgProgress->StartModal();
  }
  // Update object count
  int numAlbums = m_vecItems.Size();
  int iSkipped = 0;
  if (numAlbums)
  {
    // check for parent dir
    // check for "all" items
    // they should always be the first two items
    for (int i = 0; i <= (numAlbums>=2 ? 1 : 0); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (
        pItem->IsParentFolder() || /* parent folder */
        pItem->GetLabel().Equals(g_localizeStrings.Get(15102)) ||  /* all albums  */
        pItem->GetLabel().Equals(g_localizeStrings.Get(15103)) ||  /* all artists */
        pItem->GetLabel().Equals(g_localizeStrings.Get(15104)) ||  /* all songs   */
        pItem->GetLabel().Equals(g_localizeStrings.Get(15105))     /* all genres  */
        )
      {
        numAlbums--;
        iSkipped++;
      }
    }

    for (int i = 0; i < (int)numAlbums; i++)
    {
      CStdString strLine;
      strLine.Format("%s (%i of %i)", g_localizeStrings.Get(20098).c_str(), i + 1, numAlbums);
      m_dlgProgress->SetLine(0, strLine);
      m_dlgProgress->SetLine(1, m_vecItems[i + iSkipped]->GetLabel());
      // m_dlgProgress->SetLine(1, m_vecItems[i]->GetLabel());
      m_dlgProgress->SetLine(2, "");
      m_dlgProgress->Progress();
      // OnInfo(i,false);
      OnInfo(i + iSkipped, false);
      CGUIMessage msg(GUI_MSG_REFRESH_THUMBS,0,0);
      g_graphicsContext.SendMessage(msg);
      if (m_dlgProgress->IsCanceled())
        break;
    }
  }
  m_dlgProgress->Close();
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIWindowMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem, bool bShowInfo)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  OnInfo(m_vecItems[iItem], bShowInfo);
}

void CGUIWindowMusicBase::OnInfo(CFileItem *pItem, bool bShowInfo)
{
  if (pItem->m_bIsFolder && pItem->IsParentFolder()) return ;
  if (!pItem->m_bIsFolder)
  {
    ShowSongInfo(pItem);
    return;
  }

  if (pItem->IsVideoDb())
  {
    OnContextButton(m_viewControl.GetSelectedItem(), CONTEXT_BUTTON_INFO); // nasty but it is the same item i promise :)
    return;
  }

  // show dialog box indicating we're searching the album name
  if (m_dlgProgress && bShowInfo)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, 501);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
    if (m_dlgProgress->IsCanceled()) return ;
  }

  CStdString strPath = pItem->m_strPath;

  // Try to find an album to lookup from the current item
  CAlbum album;
  if (pItem->IsMusicDb())
  {
    DIRECTORY::MUSICDATABASEDIRECTORY::CQueryParams params;
    DIRECTORY::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->m_strPath, params);
    album.idAlbum = params.GetAlbumId();
    album.strAlbum = pItem->GetMusicInfoTag()->GetAlbum();
    album.strArtist = pItem->GetMusicInfoTag()->GetArtist();

    // we're going to need it's path as well (we assume that there's only one) - this is for
    // assigning thumbs to folders, and obtaining the local folder.jpg
    m_musicdatabase.GetAlbumPath(album.idAlbum, strPath);
  }
  else
  { // lookup is done on a folder - find the albums in the folder
    CFileItemList items;
    GetDirectory(strPath, items);

    // check the first song we find in the folder, and grab it's album info
    bool foundAlbum(false);
    for (int i = 0; i < items.Size() && !foundAlbum; i++)
    {
      CFileItem* pItem = items[i];
      pItem->LoadMusicTag();
      if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded() && !pItem->GetMusicInfoTag()->GetAlbum().IsEmpty())
      {
        // great, have a song - use it.
        CSong song(*pItem->GetMusicInfoTag());
        // this function won't be needed if/when the tag has idSong information
        if (!m_musicdatabase.GetAlbumFromSong(song, album))
        { // album isn't in the database - construct it from the tag info we have
          CMusicInfoTag *tag = pItem->GetMusicInfoTag();
          album.strAlbum = tag->GetAlbum();
          album.strArtist = tag->GetAlbumArtist().IsEmpty() ? tag->GetArtist() : tag->GetAlbumArtist();
          album.idAlbum = -1; // the -1 indicates it's not in the database
        }
        foundAlbum = true;
      }
    }
    if (!foundAlbum)
    {
      CLog::Log(LOGINFO, "%s called on a folder containing no songs with tag info - nothing can be done", __FUNCTION__);
      if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();
      return;
    }
  }

  if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();

  ShowAlbumInfo(album, strPath, false, bShowInfo);
}

void CGUIWindowMusicBase::OnManualAlbumInfo()
{
  CAlbum album;
  album.idAlbum = -1; // not in the db
  if (!CGUIDialogKeyboard::ShowAndGetInput(album.strAlbum, g_localizeStrings.Get(16011), false)) return;

  CStdString strNewArtist = "";
  if (!CGUIDialogKeyboard::ShowAndGetInput(album.strArtist, g_localizeStrings.Get(16025), false)) return;
  ShowAlbumInfo(album,"",true);
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CAlbum& album, const CStdString& path, bool bRefresh, bool bShowInfo)
{
  bool saveDb = album.idAlbum != -1;
  if (!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() && !g_passwordManager.bMasterUser)
    saveDb = false;

  // check cache
  CAlbum albumInfo;
  VECSONGS albumSongs;
  if (!bRefresh && m_musicdatabase.GetAlbumInfo(album.idAlbum, albumInfo, albumSongs))
  {
    if (!bShowInfo)
      return;

    CGUIWindowMusicInfo *pDlgAlbumInfo = (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
      pDlgAlbumInfo->SetAlbum(albumInfo, albumSongs, path);
      if (bShowInfo)
        pDlgAlbumInfo->DoModal();
      else
        pDlgAlbumInfo->RefreshThumb();  // downloads the thumb if we don't already have one

      if (!pDlgAlbumInfo->NeedRefresh())
      {
        if (pDlgAlbumInfo->HasUpdatedThumb())
          UpdateThumb(albumInfo, path);
        return;
      }
      bRefresh = true;
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  // find album info
  CMusicAlbumInfo info;
  if (FindAlbumInfo(album.strAlbum, album.strArtist, info, bShowInfo ? (bRefresh ? SELECTION_FORCED : SELECTION_ALLOWED) : SELECTION_AUTO))
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

      // ok, show album info
      CGUIWindowMusicInfo *pDlgAlbumInfo = (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
      if (pDlgAlbumInfo)
      {
        pDlgAlbumInfo->SetAlbum(info.GetAlbum(), info.GetSongs(), path);
        if (bShowInfo)
          pDlgAlbumInfo->DoModal();
        else
          pDlgAlbumInfo->RefreshThumb();  // downloads the thumb if we don't already have one

        CAlbum albumInfo = info.GetAlbum();
        albumInfo.idAlbum = album.idAlbum;
        if (pDlgAlbumInfo->HasUpdatedThumb())
          UpdateThumb(albumInfo, path);

        if (pDlgAlbumInfo->NeedRefresh())
        {
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
  CGUIDialogSongInfo *dialog = (CGUIDialogSongInfo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_SONG_INFO);
  if (dialog)
  {
    if (!pItem->IsMusicDb())
      pItem->LoadMusicTag();
    if (!pItem->HasMusicInfoTag())
      return;

    dialog->SetSong(pItem);
    dialog->DoModal(GetID());
    if (dialog->NeedsUpdate())
    { // update our file list
      m_vecItems.RemoveDiscCache();
      Update(m_vecItems.m_strPath);
    }
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
  DWORD dwStartTick = timeGetTime();

  OnRetrieveMusicInfo(m_vecItems);

  CLog::Log(LOGDEBUG, "RetrieveMusicInfo() took %lu msec", timeGetTime()-dwStartTick);
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;

  int iOldSize=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size();

  // add item 2 playlist
  CFileItem item(*m_vecItems[iItem]);

  if (item.IsRAR() || item.IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item.CanQueue())
    item.SetCanQueue(true);

  CLog::Log(LOGDEBUG, "Adding file %s%s to music playlist", item.m_strPath.c_str(), item.m_bIsFolder ? " (folder) " : "");
  CFileItemList queuedItems;
  AddItemToPlayList(&item, queuedItems);

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
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->IsMusicDb() && pItem->m_bIsFolder && !pItem->IsParentFolder())
  { // we have a music database folder, just grab the "all" item underneath it
    CMusicDatabaseDirectory dir;
    if (!dir.ContainsSongs(pItem->m_strPath))
    { // grab the ALL item in this category
      // Genres will still require 2 lookups, and queuing the entire Genre folder
      // will require 3 lookups (genre, artist, album)
      CFileItem item(pItem->m_strPath + "-1/", true);
      item.SetCanQueue(true); // workaround for CanQueue() check above
      AddItemToPlayList(&item, queuedItems);
      return;
    }
  }
  if (pItem->m_bIsFolder || (m_gWindowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
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
    GetDirectory(pItem->m_strPath, items);
    //OnRetrieveMusicInfo(items);
    SortItems(items);
    for (int i = 0; i < items.Size(); ++i)
      AddItemToPlayList(items[i], queuedItems);
  }
  else
  {
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if ( NULL != pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
          AddItemToPlayList(&playlist[i], queuedItems);
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(new CFileItem(*pItem));
    }
    else if (!pItem->IsNFO() && pItem->IsAudio())
    {
      CFileItem *itemCheck = queuedItems.Get(pItem->m_strPath);
      if (!itemCheck || itemCheck->m_lStartOffset != pItem->m_lStartOffset)
      { // add item
        CLog::Log(LOGDEBUG, "Adding item (%s) to playlist", pItem->m_strPath.c_str());
        queuedItems.Add(new CFileItem(*pItem));
      }
    }
  }
}

void CGUIWindowMusicBase::UpdateButtons()
{
  // Update window selection control

  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_graphicsContext.SendMessage(msg);

  // Add labels to the window selection
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(g_localizeStrings.Get(744)); // Files
  g_graphicsContext.SendMessage(msg2);

  msg2.SetLabel(g_localizeStrings.Get(15100)); // Library
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_stSettings.m_iMyMusicStartWindow - WINDOW_MUSIC_FILES);

  CGUIMediaWindow::UpdateButtons();
}

bool CGUIWindowMusicBase::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album, ALLOW_SELECTION allowSelection)
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet")) return false;

  // show dialog box indicating we're searching the album
  if (m_dlgProgress && allowSelection != SELECTION_AUTO)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, strAlbum);
    m_dlgProgress->SetLine(1, strArtist);
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
  }

  try
  {
    Sleep(1);
    CMusicInfoScraper scraper;
    scraper.FindAlbuminfo(strAlbum, strArtist);

    while (!scraper.Completed())
    {
      if (m_dlgProgress && m_dlgProgress->IsDialogRunning())
      {
        if (m_dlgProgress->IsCanceled())
          scraper.Cancel();
        m_dlgProgress->Progress();
      }
      Sleep(1);
    }

    if (scraper.Successfull())
    {
      // did we found at least 1 album?
      int iAlbumCount = scraper.GetAlbumCount();
      if (iAlbumCount >= 1)
      {
        //yes
        // if we found more then 1 album, let user choose one
        int iSelectedAlbum = 0;
        if (iAlbumCount > 1)
        {
          //show dialog with all albums found
          CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
            pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
            pDlg->Reset();
            pDlg->EnableButton(true);
            pDlg->SetButtonLabel(413); // manual

            int bestMatch = -1;
            double minRelevance = (allowSelection == SELECTION_AUTO) ? 0.95 : 1.0;
            double bestRelevance = 0;
            double secondBestRelevance = 0;
            for (int i = 0; i < iAlbumCount; ++i)
            {
              CMusicAlbumInfo& info = scraper.GetAlbum(i);
              double relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, info.GetAlbum().strArtist, strArtist);

              // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
              // otherwise, perfect matches only
              if (relevance >= max(minRelevance, bestRelevance))
              { // we auto-select the best of these
                secondBestRelevance = bestRelevance;
                bestRelevance = relevance;
                bestMatch = i;
              }

              // set the label to [relevance]  album - artist
              CStdString strTemp;
              strTemp.Format("[%0.2f]  %s", relevance, info.GetTitle2());
              CFileItem item(strTemp);
              item.m_idepth = i; // use this to hold the index of the album in the scraper
              pDlg->Add(&item);
            }
            if (bestMatch > -1 && bestRelevance != secondBestRelevance && allowSelection != SELECTION_FORCED)
            { // autochoose the single best matching item
              iSelectedAlbum = bestMatch;
            }
            else if (allowSelection == SELECTION_AUTO)
            { //  nothing found, or two best matches to choose from
              return false;
            }
            else
            { // let the user choose
              pDlg->Sort(false);
              pDlg->DoModal();

              // and wait till user selects one
              if (pDlg->GetSelectedLabel() < 0) 
              { // none chosen
                if (!pDlg->IsButtonPressed()) return false;
                // manual button pressed
                CStdString strNewAlbum = strAlbum;
                if (!CGUIDialogKeyboard::ShowAndGetInput(strNewAlbum, g_localizeStrings.Get(16011), false)) return false;
                if (strNewAlbum == "") return false;

                CStdString strNewArtist = strArtist;
                if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

                if (m_dlgProgress)
                {
                  m_dlgProgress->SetLine(0, strNewAlbum);
                  m_dlgProgress->SetLine(1, strNewArtist);
                  m_dlgProgress->Progress();
                }
                return FindAlbumInfo(strNewAlbum, strNewArtist, album, allowSelection);
              }
              iSelectedAlbum = pDlg->GetSelectedItem().m_idepth;
            }
          }
        }

        // ok, downloading the album info
        scraper.LoadAlbuminfo(iSelectedAlbum);

        while (!scraper.Completed())
        {
          if (m_dlgProgress)
          {
            if (m_dlgProgress->IsCanceled())
              scraper.Cancel();
            m_dlgProgress->Progress();
          }
          Sleep(1);
        }

        if (scraper.Successfull())
          album = scraper.GetAlbum(iSelectedAlbum);

        return scraper.Successfull();
      }
      else
      { // no albums found
        CGUIDialogOK::ShowAndGetInput(185, 0, 187, 0);
        return false;
      }
    }

    if (!scraper.IsCanceled() && allowSelection != SELECTION_AUTO)
    { // unable 2 connect to www.allmusic.com
      CGUIDialogOK::ShowAndGetInput(185, 0, 499, 0);
    }
  }
  catch (...)
  {
    if (m_dlgProgress && m_dlgProgress->IsDialogRunning())
      m_dlgProgress->Close();

    CLog::Log(LOGERROR, "Exception while downloading album info");
  }
  return false;
}

void CGUIWindowMusicBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;

  if (item && !item->IsParentFolder())
  {
    if (item->GetExtraInfo().Equals("lastfmloved"))
    {
      buttons.Add(CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM, 15295); //unlove
    }
    else if (item->GetExtraInfo().Equals("lastfmbanned"))
    {
      buttons.Add(CONTEXT_BUTTON_LASTFM_UNBAN_ITEM, 15296); //unban
    }
    else if (!item->GetExtraInfo().Equals("lastfmitem"))
    {
      buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347); //queue

      // allow a folder to be ad-hoc queued and played by the default player
      if (item->m_bIsFolder || (item->IsPlayList() && !g_advancedSettings.m_playlistAsFolders))
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
      else
      { // check what players we have, if we have multiple display play with option
        VECPLAYERCORES vecCores;
        CPlayerCoreFactory::GetPlayers(*item, vecCores);
        if (vecCores.size() >= 1)
          buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...
      }

      if (item->IsSmartPlayList() || m_vecItems.IsSmartPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
      else if (item->IsPlayList() || m_vecItems.IsPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT, 586);  
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowMusicBase::GetNonContextButtons(CContextButtons &buttons)
{
  if (!m_vecItems.IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);
}

bool CGUIWindowMusicBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
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
      ShowSongInfo(m_vecItems[itemNumber]);
      return true;
    }

  case CONTEXT_BUTTON_EDIT:
    {
      CStdString playlist = m_vecItems[itemNumber]->IsPlayList() ? m_vecItems[itemNumber]->m_strPath : m_vecItems.m_strPath; // save path as activatewindow will destroy our items
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR, playlist);
      return true;
    }
    
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      CStdString playlist = m_vecItems[itemNumber]->IsSmartPlayList() ? m_vecItems[itemNumber]->m_strPath : m_vecItems.m_strPath; // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist))
      { // need to update
        m_vecItems.RemoveDiscCache();
        Update(m_vecItems.m_strPath);
      }
      return true;
    }

  case CONTEXT_BUTTON_PLAY_ITEM:
    PlayItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_WITH:
    {
      VECPLAYERCORES vecCores;  // base class?
      CPlayerCoreFactory::GetPlayers(*m_vecItems[itemNumber], vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
      if( g_application.m_eForcedNextPlayer != EPC_NONE )
        OnClick(itemNumber);
      return true;
    }

  case CONTEXT_BUTTON_STOP_SCANNING:
    { 
      CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (scanner)
        scanner->StopScanning();
      return true;
    }

  case CONTEXT_BUTTON_NOW_PLAYING:
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;

  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;

  case CONTEXT_BUTTON_SETTINGS:
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
    return true;
  case CONTEXT_BUTTON_LASTFM_UNBAN_ITEM:
    if (CLastFmManager::GetInstance()->Unban(*m_vecItems[itemNumber]->GetMusicInfoTag()))
    {
      g_directoryCache.ClearDirectory(m_vecItems.m_strPath);
      m_vecItems.RemoveDiscCache();
      Update(m_vecItems.m_strPath);
    }
    return true;
  case CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM:
    if (CLastFmManager::GetInstance()->Unlove(*m_vecItems[itemNumber]->GetMusicInfoTag()))
    {
      g_directoryCache.ClearDirectory(m_vecItems.m_strPath);
      m_vecItems.RemoveDiscCache();
      Update(m_vecItems.m_strPath);
    }
    return true;
  default:
    break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowMusicBase::OnRipCD()
{
  CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CCDDARipper ripper;
      ripper.RipCD();
#endif
    }
    else
    {
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      pDlgOK->SetHeading(257); // Error
      pDlgOK->SetLine(0, g_localizeStrings.Get(20099)); //
      pDlgOK->SetLine(1, ""); //
      pDlgOK->SetLine(2, "");
      pDlgOK->DoModal();
    }
  }
}

void CGUIWindowMusicBase::OnRipTrack(int iItem)
{
  CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CCDDARipper ripper;
      ripper.RipTrack(m_vecItems[iItem]);
#endif
    }
    else
    {
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      pDlgOK->SetHeading(257); // Error
      pDlgOK->SetLine(0, g_localizeStrings.Get(20099)); //
      pDlgOK->SetLine(1, ""); //
      pDlgOK->SetLine(2, "");
      pDlgOK->DoModal();
    }
  }
}

void CGUIWindowMusicBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the current playlist

  const CFileItem* pItem = m_vecItems[iItem];

  // special case for DAAP playlist folders
  bool bIsDAAPplaylist = false;
#ifdef HAS_FILESYSTEM_DAAP
  if (pItem->IsDAAP() && pItem->m_bIsFolder)
  {
    CDAAPDirectory dirDAAP;
    if (dirDAAP.GetCurrLevel(pItem->m_strPath) == 0)
      bIsDAAPplaylist = true;
  }
#endif
  // if its a folder, build a playlist
  if (pItem->m_bIsFolder || (m_gWindowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
  {
    CFileItem item(*m_vecItems[iItem]);

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item.CanQueue())
      item.SetCanQueue(true);

    // skip ".."
    if (item.IsParentFolder())
      return;

    CFileItemList queuedItems;
    AddItemToPlayList(&item, queuedItems);
    if (g_partyModeManager.IsEnabled())
    {
      g_partyModeManager.AddUserSongs(queuedItems, true);
      return;
    }

    /*
    CStdString strPlayListDirectory = m_vecItems.m_strPath;
    if (CUtil::HasSlashAtEnd(strPlayListDirectory))
      strPlayListDirectory.Delete(strPlayListDirectory.size() - 1);
    */

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);

    // activate the playlist window if its not activated yet
    if (bIsDAAPplaylist && GetID() == (DWORD) m_gWindowManager.GetActiveWindow())
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

    // play!
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->m_strPath);
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
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistmusic://");
    // activate the playlist window if its not activated yet
    if (GetID() == (DWORD) m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
  }
}

bool CGUIWindowMusicBase::OnPlayMedia(int iItem)
{
  CFileItem* pItem = m_vecItems[iItem];
  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("music"))
    {
      Update("");
      return true;
    }
    return false;
  }

  // party mode
  if (g_partyModeManager.IsEnabled() && !pItem->IsLastFM())
  {
    CPlayList playlistTemp;
    CPlayList::CPlayListItem playlistItem;
    CUtil::ConvertFileItemToPlayListItem(m_vecItems[iItem], playlistItem);
    playlistTemp.Add(playlistItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }
  else if (!pItem->IsPlayList() && !pItem->IsInternetStream())
  { // single music file - if we get here then we have autoplaynextitem turned off, but we
    // still want to use the playlist player in order to handle more queued items following etc.
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
  if (!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() && !g_passwordManager.bMasterUser)
  {
    saveDb = false;
    saveDirThumb = false;
  }

  CStdString albumThumb(CUtil::GetCachedAlbumThumb(album.strAlbum, album.strArtist));

  // Update the thumb in the music database (songs + albums)
  CStdString albumPath(path);
  if (saveDb && CFile::Exists(albumThumb))
    m_musicdatabase.SaveAlbumThumb(album.idAlbum, albumThumb);

  // Update currently playing song if it's from the same album.  This is necessary as when the album
  // first gets it's cover, the info manager's item doesn't have the updated information (so will be
  // sending a blank thumb to the skin.)
  if (g_application.IsPlayingAudio())
  {
    CStdString strSongFolder;
    const CMusicInfoTag* tag=g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      // really, this may not be enough as it is to reliably update this item.  eg think of various artists albums
      // that aren't tagged as such (and aren't yet scanned).  But we probably can't do anything better than this
      // in that case
      if (album.strAlbum == tag->GetAlbum() && (album.strArtist == tag->GetAlbumArtist() || album.strArtist == tag->GetArtist()))
        g_infoManager.SetCurrentAlbumThumb(albumThumb);
    }
  }

  // Save this thumb as the directory thumb if it's the only album in the folder (files view nicety)
  // We do this by grabbing all the songs in the folder, and checking to see whether they come
  // from the same album.
  if (saveDirThumb && CFile::Exists(albumThumb) && !albumPath.IsEmpty() && !CUtil::IsCDDA(albumPath))
  {
    CFileItemList items;
    GetDirectory(albumPath, items);
    OnRetrieveMusicInfo(items);
    VECSONGS songs;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItem *item = items[i];
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->Loaded())
      {
        CSong song(*item->GetMusicInfoTag());
        songs.push_back(song);
      }
    }
    CMusicInfoScanner::CheckForVariousArtists(songs);
    CStdString album, artist;
    if (CMusicInfoScanner::HasSingleAlbum(songs, album, artist))
    { // can cache as the folder thumb
      CStdString folderThumb(CUtil::GetCachedMusicThumb(albumPath));
      ::CopyFile(albumThumb, folderThumb, false);
    }
  }

  // update the file listing
  int iSelectedItem = m_viewControl.GetSelectedItem();
  if (iSelectedItem >= 0 && m_vecItems[iSelectedItem])
  {
    CFileItem* pSelectedItem=m_vecItems[iSelectedItem];
    if (pSelectedItem->m_bIsFolder)
    {
      // refresh only the icon of
      // the current folder
      pSelectedItem->FreeMemory();
      if (!pSelectedItem->HasThumbnail())
        pSelectedItem->SetThumbnailImage(albumThumb);
      pSelectedItem->FillInDefaultIcon();
    }
    else
    {
      // Refresh all items
      m_vecItems.RemoveDiscCache();
      Update(m_vecItems.m_strPath);
    }

    //  Do we have to autoswitch to the thumb control?
    m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
    UpdateButtons();
  }
}

void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{
/*
  if (items.GetFolderCount()==items.Size() || items.IsMusicDb() || (!g_guiSettings.GetBool("musicfiles.usetags") && !items.IsCDDA()))
    return;
*/
  // Start the music info loader thread
  m_musicInfoLoader.SetProgressCallback(m_dlgProgress);
  m_musicInfoLoader.Load(items);

  bool bShowProgress=!m_gWindowManager.HasModalDialog();
  bool bProgressVisible=false;

  DWORD dwTick=timeGetTime();

printf("Scanning\n");
  while (m_musicInfoLoader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      DWORD dwElapsed=timeGetTime()-dwTick;

      if (!bProgressVisible && dwElapsed>1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.m_strPath);
        CStdString strStrippedPath;
        url.GetURLWithoutUserDetails(strStrippedPath);
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
printf("Scanning done\n");

  if (bProgressVisible && m_dlgProgress)
    m_dlgProgress->Close();
}

bool CGUIWindowMusicBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.SetThumbnailImage("");
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory,items);
  if (bResult)
    items.SetMusicThumb();

  // add in the "New Playlist" item if we're in the playlists folder
  if (items.m_strPath == "special://musicplaylists/" && !items.Contains("newplaylist://"))
  {
    CFileItem* newPlaylist = new CFileItem(g_settings.GetUserDataItem("PartyMode.xsp"),false);
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

    newPlaylist = new CFileItem("newplaylist://", false);
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);

    newPlaylist = new CFileItem("newsmartplaylist://music", false);
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  return bResult;
}
