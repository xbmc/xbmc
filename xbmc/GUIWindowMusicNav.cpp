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
#include "GUIWindowMusicNav.h"
#include "util.h"
#include "Utils/GUIInfoManager.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIPassword.h"
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "GUILabelControl.h"
#include "GUIFontManager.h"
#endif
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileBrowser.h"
#include "Picture.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "PartyModeManager.h"
#include "PlaylistFactory.h"

using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace MUSICDATABASEDIRECTORY;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_BTNMANUALINFO     17
#define CONTROL_BTN_FILTER        19
#define CONTROL_LABELEMPTY        18

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_NAV, "MyMusicNav.xml")
{
  m_vecItems.m_strPath = "?";
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowMusicNav::~CGUIWindowMusicNav(void)
{
}

bool CGUIWindowMusicNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems.m_strPath = "?";
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check for valid quickpath parameter
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }

      // is this the first time the window is opened?
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_stSettings.m_szDefaultMusicLibView;
        m_vecItems.m_strPath = strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      if (!strDestination.IsEmpty())
      {
        if (strDestination.Equals("$ROOT") || strDestination.Equals("Root"))
        {
          m_vecItems.m_strPath = "";
        }
        else if (strDestination.Equals("Genres"))
        {
          m_vecItems.m_strPath = "musicdb://1/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Artists"))
        {
          m_vecItems.m_strPath = "musicdb://2/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Albums"))
        {
          m_vecItems.m_strPath = "musicdb://3/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Songs"))
        {
          m_vecItems.m_strPath = "musicdb://4/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Top100"))
        {
          m_vecItems.m_strPath = "musicdb://5/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Top100Songs"))
        {
          m_vecItems.m_strPath = "musicdb://5/2/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Top100Albums"))
        {
          m_vecItems.m_strPath = "musicdb://5/1/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("RecentlyAddedAlbums"))
        {
          m_vecItems.m_strPath = "musicdb://6/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("RecentlyPlayedAlbums"))
        {
          m_vecItems.m_strPath = "musicdb://7/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Compilations"))
        {
          m_vecItems.m_strPath = "musicdb://8/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else if (strDestination.Equals("Playlists"))
        {
          m_vecItems.m_strPath = "special://musicplaylists/";
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) is not valid!", strDestination.c_str());
          break;
        }
      }

      DisplayEmptyDatabaseMessage(false); // reset message state

      if (!CGUIWindowMusicBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      DisplayEmptyDatabaseMessage(m_musicdatabase.GetSongsCount() <= 0);

      if (m_bDisplayEmptyDatabaseMessage)
      {
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        Update(m_vecItems.m_strPath);  // Will remove content from the list/thumb control
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
          g_partyModeManager.Enable();

          // Playlist directory is the root of the playlist window
          if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistmusic://");

          return true;
        }
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNMANUALINFO)
      {
        OnManualAlbumInfo();
        return true;
      }
      else if (iControl == CONTROL_BTN_FILTER)
      {
        CGUIDialogKeyboard::ShowAndGetFilter(m_filter, false);
        return true;
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_FILTER_ITEMS && IsActive())
      {
        m_filter = message.GetStringParam();
        m_filter.TrimLeft().ToLower();
        OnFilterItems();
      }
      if (message.GetParam1() == GUI_MSG_SEARCH_UPDATE && IsActive())
      {
        m_search = message.GetStringParam();
        CUtil::URLEncode(m_search);
        if (!m_search.IsEmpty())
        {
          CStdString path = "musicsearch://" + m_search + "/";
          m_history.ClearPathHistory();
          Update(path);
        }
      }
    }
  }
  return CGUIWindowMusicBase::OnMessage(message);
}

CStdString CGUIWindowMusicNav::GetQuickpathName(const CStdString& strPath) const
{
  if (strPath.Equals("musicdb://1/"))
    return "Genres";
  else if (strPath.Equals("musicdb://2/"))
    return "Artists";
  else if (strPath.Equals("musicdb://3/"))
    return "Albums";
  else if (strPath.Equals("musicdb://4/"))
    return "Songs";
  else if (strPath.Equals("musicdb://5/"))
    return "Top100";
  else if (strPath.Equals("musicdb://5/2/"))
    return "Top100Songs";
  else if (strPath.Equals("musicdb://5/1/"))
    return "Top100Albums";
  else if (strPath.Equals("musicdb://6/"))
    return "RecentlyAddedAlbums";
  else if (strPath.Equals("musicdb://7/"))
    return "RecentlyPlayedAlbums";
  else if (strPath.Equals("musicdb://8/"))
    return "Compilations";
  else if (strPath.Equals("special://musicplaylists/"))
    return "Playlists";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowMusicNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowMusicNav::OnClick(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return false;

  CFileItem *item = m_vecItems[iItem];
  if (item->m_strPath.Left(14) == "musicsearch://")
  {
    // popup the search window - it'll take care of the view updating
    CStdString search(m_search);
    CUtil::UrlDecode(search);
    CGUIDialogKeyboard::ShowAndGetFilter(search, true);
    return true;
  }
  return CGUIWindowMusicBase::OnClick(iItem);
}

bool CGUIWindowMusicNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_bDisplayEmptyDatabaseMessage)
    return true;

  CFileItem directory(strDirectory, true);

  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  bool bResult = CGUIWindowMusicBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (directory.IsPlayList())
      OnRetrieveMusicInfo(items);
    else if (!items.IsMusicDb())  // don't need to do this for playlist, as OnRetrieveMusicInfo() should ideally set thumbs
    {
      items.SetCachedMusicThumbs();
      m_thumbLoader.Load(m_vecItems);
    }
  }

  DIRECTORY::CMusicDatabaseDirectory dir;
  DIRECTORY::MUSICDATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(strDirectory);
  if (node == DIRECTORY::MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM)
    g_infoManager.m_content = "albums";
  else if (node == DIRECTORY::MUSICDATABASEDIRECTORY::NODE_TYPE_ARTIST)
    g_infoManager.m_content = "artists";
  else if (node == DIRECTORY::MUSICDATABASEDIRECTORY::NODE_TYPE_SONG)
    g_infoManager.m_content = "songs";
  else
    g_infoManager.m_content = "";

  return bResult;
}

void CGUIWindowMusicNav::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->IsParentFolder()) iItems--;
      if (pItem->m_strPath.Left(4).Equals("/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems.Size() > 2 &&
      m_vecItems[m_vecItems.Size()-1]->m_strPath.Left(4).Equals("/-1/"))
      iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  // set the filter label
  CStdString strLabel;

  // "Playlists"
  if (m_vecItems.m_strPath.Equals("special://musicplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems.IsPlayList())
  {
    // get playlist name from path
    CStdString strDummy;
    CUtil::Split(m_vecItems.m_strPath, strDummy, strLabel);
  }
  // everything else is from a musicdb:// path
  else
  {
    CMusicDatabaseDirectory dir;
    dir.GetLabel(m_vecItems.m_strPath, strLabel);
  }

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTN_FILTER, !m_filter.IsEmpty());
}

/// \brief Search for songs, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  m_musicdatabase.Search(strSearch, items);
}

void CGUIWindowMusicNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems.IsVirtualDirectoryRoot())
    return;

  CGUIWindowMusicBase::PlayItem(iItem);
}

void CGUIWindowMusicNav::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList && !GetControl(CONTROL_LABELEMPTY))
  {
    CLabelInfo info;
    info.align = XBFONT_CENTER_X | XBFONT_CENTER_Y;
    info.font = g_fontManager.GetFont("font13");
    info.textColor = 0xffffffff;
    CGUILabelControl *pLabel = new CGUILabelControl(GetID(),CONTROL_LABELEMPTY,pList->GetXPosition(),pList->GetYPosition(),pList->GetWidth(),pList->GetHeight(),"",info,false);
    pLabel->SetAnimations(pList->GetAnimations());
    Add(pLabel);
  }
#endif

  CGUIWindowMusicBase::OnWindowLoaded();
}

void CGUIWindowMusicNav::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // mark the item
  bool bSelected = m_vecItems[iItem]->IsSelected(); // item maybe selected (playlistitem)
  m_vecItems[iItem]->Select(true);
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();
  // add the needed buttons
  int btn_Queue       = 0;  // Queue Item
  int btn_PlayWith    = 0;  // Play using alternate player
  int btn_EditPlaylist = 0; // Edit a playlist
  int btn_Info        = 0;  // Music Information
  int btn_InfoAll     = 0;  // Query Information for all albums
  int btn_GoToRoot    = 0;
  int btn_NowPlaying  = 0;  // Now Playing... very bottom of context accessible

  // directory tests
  CMusicDatabaseDirectory dir;

  // check what players we have, if we have multiple display play with option
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

  // turn off info/queue/play/set artist thumb if the current item is goto parent ..
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
  bool bPlaylists = m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()) || m_vecItems.m_strPath.Equals("special://musicplaylists/");
  if (!bIsGotoParent && (dir.GetDirectoryType(m_vecItems.m_strPath) != NODE_TYPE_ROOT || bPlaylists))
  {
    // allow queue for anything but root
    if (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList() || m_vecItems[iItem]->IsAudio())
      btn_Queue = pMenu->AddButton(13347);

    // allow a folder to be ad-hoc queued and played by the default player
    if (m_vecItems[iItem]->m_bIsFolder || (m_vecItems[iItem]->IsPlayList() && !g_advancedSettings.m_playlistAsFolders))
      btn_PlayWith = pMenu->AddButton(208);
    else if (vecCores.size() >= 1)
      btn_PlayWith = pMenu->AddButton(15213);

    if ((m_vecItems[iItem]->IsPlayList() && !m_vecItems[iItem]->IsSmartPlayList()) ||
        (m_vecItems.IsPlayList() && !m_vecItems[iItem]->IsSmartPlayList()))
      btn_EditPlaylist = pMenu->AddButton(586);

    // enable music info button only in album view
    if (dir.HasAlbumInfo(m_vecItems[iItem]->m_strPath) && !dir.IsAllItem(m_vecItems[iItem]->m_strPath) && m_vecItems[iItem]->m_bIsFolder)
      btn_Info = pMenu->AddButton(13351);

    // enable query all albums button only in album view
    if (dir.HasAlbumInfo(m_vecItems[iItem]->m_strPath) && !dir.IsAllItem(m_vecItems[iItem]->m_strPath) && m_vecItems[iItem]->m_bIsFolder)
      btn_InfoAll = pMenu->AddButton(20059);
  }

  // turn off set artist image if not at artist listing.
  // (uses file browser to pick an image)
  int btn_Thumb = 0;  // Set Artist Thumb
  if (dir.IsArtistDir(m_vecItems[iItem]->m_strPath) && !dir.IsAllItem(m_vecItems[iItem]->m_strPath))
    btn_Thumb = pMenu->AddButton(13359);

  //Set default or clear default
  int btn_Default = 0;
  int btn_ClearDefault=0;
  NODE_TYPE nodetype = dir.GetDirectoryType(m_vecItems[iItem]->m_strPath);
  if (
    !bIsGotoParent && 
    !bPlaylists && 
    (nodetype == NODE_TYPE_ROOT || nodetype == NODE_TYPE_OVERVIEW || nodetype == NODE_TYPE_TOP100) 
  )
  {
    if (!m_vecItems[iItem]->m_strPath.Equals(g_stSettings.m_szDefaultMusicLibView))
      btn_Default = pMenu->AddButton(13335); // set default
    if (strcmp(g_stSettings.m_szDefaultMusicLibView, ""))
      btn_ClearDefault = pMenu->AddButton(13403); // clear default
  }

  // noncontextual buttons
  int btn_Settings = pMenu->AddButton(5);     // Settings...

  if (dir.GetDirectoryType(m_vecItems.m_strPath) != NODE_TYPE_ROOT)
    btn_GoToRoot = pMenu->AddButton(20128);

  // if the Now Playing item is still not in the list, add it here
  if (btn_NowPlaying == 0 && g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
    btn_NowPlaying = pMenu->AddButton(13350);

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btn = pMenu->GetButton();
  if (btn > 0)
  {
    if (btn == btn_Info) // Music Information
    {
      OnInfo(iItem);
    }
    else if (btn == btn_InfoAll) // Music Information
    {
      OnInfoAll(iItem);
    }
    else if (btn == btn_PlayWith)
    {
      // if folder, play with default player
      if (m_vecItems[iItem]->m_bIsFolder)
      {
        PlayItem(iItem);
      }
      else
      {
        // Play With...
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, posX, posY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
    }
    else if (btn == btn_EditPlaylist)
    {
      CStdString playlist = m_vecItems[iItem]->IsPlayList() ? m_vecItems[iItem]->m_strPath : m_vecItems.m_strPath; // save path as activatewindow will destroy our items
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR, playlist);
      return;
    }
     else if (btn == btn_Queue)  // Queue Item
    {
      OnQueueItem(iItem);
    }
    else if (btn == btn_NowPlaying)  // Now Playing...
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
      return;
    }
    else if (btn == btn_Thumb)  // Set Artist Image
    {
      SetArtistImage(iItem);
    }
    else if (btn == btn_GoToRoot)
    {
      Update("");
      return;
    }
    else if (btn == btn_Default) // Set default
    {
      strcpy(g_stSettings.m_szDefaultMusicLibView, GetQuickpathName(m_vecItems[iItem]->m_strPath).c_str());
      g_settings.Save();
    }
    else if (btn == btn_ClearDefault) // Clear default
    {
      strcpy(g_stSettings.m_szDefaultMusicLibView, "");
      g_settings.Save();
    }
    else if (btn == btn_Settings)  // Settings
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
    }
  }
  if (iItem < m_vecItems.Size())
    m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowMusicNav::SetArtistImage(int iItem)
{
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPicture;

  CStdString strPath = pItem->m_strPath;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  int nPos=strPath.ReverseFind("/");
  if (nPos>-1)
  {
    //  try to guess where the user should start
    //  browsing for the artist thumb
    VECALBUMS albums;
    long idArtist=atol(strPath.Right(strPath.size()-nPos-1));
    m_musicdatabase.GetAlbumsByArtistId(idArtist, albums);
    if (albums.size())
    {
      strPicture = albums[0].strPath;
      int iPos = -1;
      int iSlashes=0;
      char slash='/';
      if (strPicture.find('\\') != -1)
        slash = '\\';
      while ((iPos = strPicture.find(slash,iPos+1)) != -1)
        iSlashes++;

      for (unsigned int i=1;i<albums.size();++i)
      {
        int j=0;
        while (strPicture[j] == albums[i].strPath[j]) j++;
        strPicture.Delete(j,strPicture.size()-j);
      }

      if (!strPicture.Equals(albums[0].strPath))
      {
        iPos = -1;
        int iSlashes2 = 0;
        while ((iPos = strPicture.find(slash,iPos+1)) != -1)
          iSlashes2++;
        if (iSlashes == iSlashes2 && strPicture[strPicture.size()-1] != slash) // we have a partly match - happens with e.g. f:\foo\disc 1, f:\foo\disc 2 -> strPicture = f:\foo\disc
        {
          CStdString strPicture2(strPicture);
          CUtil::GetParentPath(strPicture2,strPicture);
        }
      }
      if (strPicture.size() > 2)
      {
        if ((strPicture[strPicture.size()-1] == '/' && strPicture[strPicture.size()-2] == '/') || (strPicture[1] ==':' && strPicture[2] == '\\' && strPicture.size()==3) || strPicture.IsEmpty())
          strPicture = ""; // no protocol/drive-only matching
         // need the slash for the filebrowser
        else if (!strPicture.Equals(albums[0].strPath))
          CUtil::AddSlashAtEnd(strPicture);
      }
    }
  }

  if (CGUIDialogFileBrowser::ShowAndGetImage(g_settings.m_vecMyMusicShares, g_localizeStrings.Get(20010), strPicture))
  {
    CStdString thumb(pItem->GetCachedArtistThumb());
    CPicture picture;
    if (picture.DoCreateThumbnail(strPicture, thumb))
    {
      CMusicDatabaseDirectory dir;
      dir.ClearDirectoryCache(m_vecItems.m_strPath);
      Update(m_vecItems.m_strPath);
    }
    else
      CLog::Log(LOGERROR,"  Could not cache artist thumb: %s",strPicture.c_str());
  }
}

bool CGUIWindowMusicNav::GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items)
{
  CStdString strParentPath=m_history.GetParentPath();

  if (m_guiState.get() && !m_guiState->HideParentDirItems())
  {
    CFileItem *pItem = new CFileItem("..");
    pItem->m_strPath = strParentPath;
    items.Add(pItem);
  }

  items.m_strPath=strPlayList;
  CLog::Log(LOGDEBUG,"CGUIWindowMusicNav, opening playlist [%s]", strPlayList.c_str());

  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
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
      CSong song;
      song.strFileName = playlist[i].m_strPath;
      song.strTitle = CUtil::GetFileName(song.strFileName);
      song.iDuration = playlist[i].GetDuration();
      CFileItem *item = new CFileItem(song);
      items.Add(item);
    }

  }

  return true;
}

void CGUIWindowMusicNav::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowMusicNav::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }
  CGUIWindowMusicBase::Render();
}

void CGUIWindowMusicNav::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.ClearKeepPointer();
  m_unfilteredItems.Clear();
}

void CGUIWindowMusicNav::OnFilterItems()
{
  CStdString currentItem;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0)
    currentItem = m_vecItems[item]->m_strPath;

  m_viewControl.Clear();

  FilterItems(m_vecItems);

  // and update our view control + buttons
  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetSelectedItem(currentItem);
  UpdateButtons();
}

void CGUIWindowMusicNav::FilterItems(CFileItemList &items)
{
  if (m_vecItems.IsVirtualDirectoryRoot())
    return;

  items.ClearKeepPointer();
  for (int i = 0; i < m_unfilteredItems.Size(); i++)
  {
    CFileItem *item = m_unfilteredItems[i];
    if (item->IsParentFolder() || m_filter.IsEmpty())
    {
      items.Add(item);
      continue;
    }
    // TODO: Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    // though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    // where we are in the library tree.
    // Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    // but it's re-enabled on the way back out.
    CStdString match;
/*    if (item->GetFocusedLayout())
      match = item->GetFocusedLayout()->GetAllText();
    else if (item->GetLayout())
      match = item->GetLayout()->GetAllText();
    else*/
      match = item->GetLabel() + " " + item->GetLabel2();
    if (StringUtils::FindWords(match.c_str(), m_filter.c_str()))
      items.Add(item);
  }
}

void CGUIWindowMusicNav::OnFinalizeFileItems(CFileItemList &items)
{
  CGUIMediaWindow::OnFinalizeFileItems(items);
  m_unfilteredItems.AppendPointer(items);
  // now filter as necessary
  if (!m_filter.IsEmpty())
    FilterItems(items);
}