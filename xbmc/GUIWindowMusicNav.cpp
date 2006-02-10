#include "stdafx.h"
#include "GUIWindowMusicNav.h"
#include "PlayListFactory.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIPassword.h"
#include "GUIListControl.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileBrowser.h"
#include "Picture.h"
#include "FileSystem/MusicDatabaseDirectory.h"

using namespace MUSICDATABASEDIRECTORY;

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_FILTER      15
#define CONTROL_BTNSHUFFLE  21

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_NAV, "MyMusicNav.xml")
{
  m_vecItems.m_strPath = "?";
}
CGUIWindowMusicNav::~CGUIWindowMusicNav(void)
{
}

bool CGUIWindowMusicNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      //  first time
      if (m_vecItems.m_strPath == "?")
      {
        //  Setup shares we want to have

        //  Musicdb shares
        CFileItemList items;
        CDirectory::GetDirectory("musicdb://", items);
        for (int i=0; i<items.Size(); ++i)
        {
          CFileItem* item=items[i];
          CShare share;
          share.strName=item->GetLabel();
          share.strPath=item->m_strPath;
          share.m_strThumbnailImage="defaultFolderBig.png";
          m_shares.push_back(share);
        }

        //  Playlists share
        CShare share;
        share.strName=g_localizeStrings.Get(136); // Playlists
        share.strPath=CUtil::MusicPlaylistsLocation();
        share.m_strThumbnailImage="defaultFolderBig.png";
        m_shares.push_back(share);

        // setup shares and file filters
        m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
        m_rootDir.SetShares(m_shares);

        m_vecItems.m_strPath = "";
      }

      // check for valid quickpath parameter
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());

        if (strDestination.Equals("Genres"))
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
        else if (strDestination.Equals("Top100Songs"))
        {
          m_vecItems.m_strPath = "musicdb://5/2";
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
        else if (strDestination.Equals("Playlists"))
        {
          m_vecItems.m_strPath = CUtil::MusicPlaylistsLocation();
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) is not valid!", strDestination.c_str());
          break;
        }
      }

      DisplayEmptyDatabaseMessage(false); // reset message state

      CGUIWindowMusicBase::OnMessage(message);

      //  base class has opened the database, do our check
      DisplayEmptyDatabaseMessage(g_musicDatabase.GetSongsCount() <= 0);

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
      if (iControl == CONTROL_BTNSHUFFLE) // shuffle?
      {
        // inverse current playlist shuffle state but do not save!
        g_stSettings.m_bMyMusicPlaylistShuffle = !g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP);
        g_playlistPlayer.ShufflePlay(PLAYLIST_MUSIC_TEMP, g_stSettings.m_bMyMusicPlaylistShuffle);

        UpdateButtons();

        return true;
      }
    }
    break;
  }
  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_bDisplayEmptyDatabaseMessage)
    return true;

  CFileItem directory(strDirectory, true);
  if (directory.IsPlayList())
    return GetSongsFromPlayList(strDirectory, items);

  return CGUIWindowMusicBase::GetDirectory(strDirectory, items);
}

void CGUIWindowMusicNav::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    // check for parent dir
    // check for "all" items
    // they should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->IsParentFolder()) iItems--;
      if (
        pItem->GetLabel().Equals((CStdString)g_localizeStrings.Get(15102)) ||  /* all albums  */
        pItem->GetLabel().Equals((CStdString)g_localizeStrings.Get(15103)) ||  /* all artists */
        pItem->GetLabel().Equals((CStdString)g_localizeStrings.Get(15104)) ||  /* all songs   */
        pItem->GetLabel().Equals((CStdString)g_localizeStrings.Get(15105))     /* all genres  */
        )
        iItems--;
    }
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);

  // set the filter label
  CStdString strLabel;

  // "Playlists"
  if (m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems.IsPlayList())
  {
    // get playlist name from path
    CStdString strDummy;
    CUtil::Split(m_vecItems.m_strPath, strDummy, strLabel);
  }
  // everything else is from a musicdb:// path
  // for now display "Library"
  // TODO: add a "GetLabel" function to CMusicDatabase to restore filter label
  else
  {
    CMusicDatabaseDirectory dir;
    dir.GetLabel(m_vecItems.m_strPath, strLabel);
    //strLabel = g_localizeStrings.Get(15100);
  }
  
  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  // Mark the shuffle button
  if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP))
  {
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);
  }
}

bool CGUIWindowMusicNav::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];

  if (!pItem->m_bIsFolder && pItem->IsPlayList())
  { //  treat playlists like folders
    CStdString strPath=pItem->m_strPath;
    m_history.AddPath(strPath);
    Update(strPath);
    return true;
  }

  return CGUIWindowMusicBase::OnClick(iItem);
}

/// \brief Search for songs, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  // get matching genres
  VECGENRES genres;
  g_musicDatabase.GetGenresByName(strSearch, genres);

  if (genres.size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)genres.size(); i++)
    {
      CGenre& genre = genres[i]; 
      CFileItem* pItem = new CFileItem(genre);
      pItem->SetLabel("[" + strGenre + "] " + genre.strGenre);
      pItem->m_strPath.Format("musicdb://1/%ld/", genre.idGenre);
      items.Add(pItem);
    }
  }

  // get matching artists
  VECARTISTS artists;
  g_musicDatabase.GetArtistsByName(strSearch, artists);

  if (artists.size())
  {
    CStdString strArtist = g_localizeStrings.Get(484); // Artist
    for (int i = 0; i < (int)artists.size(); i++)
    {
      CArtist& artist = artists[i];
      CFileItem* pItem = new CFileItem(artist);
      pItem->SetLabel("[" + strArtist + "] " + artist.strArtist);
      pItem->m_strPath.Format("musicdb://2/%ld/", artist.idArtist);
      items.Add(pItem);
    }
  }

  // get matching albums
  VECALBUMS albums;
  g_musicDatabase.GetAlbumsByName(strSearch, albums);

  if (albums.size())
  {
    CStdString strAlbum = g_localizeStrings.Get(483); // Album
    for (int i = 0; i < (int)albums.size(); i++)
    {
      CAlbum& album = albums[i];
      CFileItem* pItem = new CFileItem(album);
      pItem->SetLabel("[" + strAlbum + "] " + album.strAlbum + " - " + album.strArtist);
      pItem->m_strPath.Format("musicdb://3/%ld/", album.idAlbum);
      items.Add(pItem);
    }
  }

  // get matching songs
  VECSONGS songs;
  g_musicDatabase.FindSongsByName(strSearch, songs, true);

  if (songs.size())
  {
    CStdString strSong = g_localizeStrings.Get(179); // Song
    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong& song = songs[i];
      CFileItem* pItem = new CFileItem(song);
      pItem->SetLabel("[" + strSong + "] " + song.strTitle + " (" + song.strAlbum + " - " + song.strArtist + ")");
      items.Add(pItem);
    }
  }
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

void CGUIWindowMusicNav::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
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
  int btn_Info     = 0;  // Music Information
  int btn_PlayWith = 0;  // Play using alternate player
  int btn_Queue    = 0;  // Queue Item

  // directory tests
  CMusicDatabaseDirectory dir;

  // check what players we have, if we have multiple display play with option
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

  // turn off info/queue/play/set artist thumb if the current item is goto parent ..
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
  if (!bIsGotoParent && dir.GetDirectoryType(m_vecItems.m_strPath) != NODE_TYPE_ROOT)
  { 
    // enable music info button
    if (dir.HasAlbumInfo(m_vecItems[iItem]->m_strPath) && !dir.IsAllItem(m_vecItems[iItem]->m_strPath))
      btn_Info = pMenu->AddButton(13351);

    if (vecCores.size() >= 1)
      btn_PlayWith = pMenu->AddButton(15213);
    // allow a folder to be ad-hoc queued and played by the default player
    else if (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList())
      btn_PlayWith = pMenu->AddButton(208);
    
    // allow queue for anything but root
    btn_Queue = pMenu->AddButton(13347);
  }

  int btn_NowPlay  = 0;  // Now Playing...
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
    btn_NowPlay = pMenu->AddButton(13350);

  // always visible
  int btn_Search = pMenu->AddButton(137);     // Search...

  // turn off set artist image if not at artist listing.
  // (uses file browser to pick an image)
  int btn_Thumb = 0;  // Set Artist Thumb  
  if (dir.IsArtistDir(m_vecItems[iItem]->m_strPath) && !dir.IsAllItem(m_vecItems[iItem]->m_strPath))
    btn_Thumb = pMenu->AddButton(13359);

  // always visible
  int btn_Settings = pMenu->AddButton(5);     // Settings...

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());

  int btn = pMenu->GetButton();
  if (btn > 0)
  {
    if (btn == btn_Info) // Music Information
    {
      OnInfo(iItem);
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
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, iPosX, iPosY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
    }
    else if (btn == btn_Queue)  // Queue Item
    {
      OnQueueItem(iItem);
    }
    else if (btn == btn_NowPlay)  // Now Playing...
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
      return;
    }
    else if (btn == btn_Search)  // Search
    {
      OnSearch();
    }
    else if (btn == btn_Thumb)  // Set Artist Image
    {
      SetArtistImage(iItem);
    }
    else if (btn == btn_Settings)  // Settings
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
      return;
    }
  }
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
    g_musicDatabase.GetAlbumsByArtistId(idArtist, albums);
    if (albums.size())
    {
      strPicture = albums[0].strPath;
      for (unsigned int i=1;i<albums.size();++i)
      {
        int j=0;
        while (strPicture[j] == albums[i].strPath[j]) j++;
        strPicture.Delete(j,strPicture.size()-j);
      }

      if (strPicture.size() > 2)
      {
        if ((strPicture[strPicture.size()-1] == '/' && strPicture[strPicture.size()-2] == '/') || (strPicture[1]==':' && strPicture[2] == '\\' && strPicture.size()==3))
          strPicture = ""; // no protocol/drive-only matching
        else if (CUtil::HasSlashAtEnd(strPicture))
          strPicture.Delete(strPicture.size()-1,1);
      }
    }
  }

  if (CGUIDialogFileBrowser::ShowAndGetFile( g_settings.m_vecMyMusicShares, ".jpg|.tbn", L"artist cover", strPicture))
  {
    CStdString strArtist = "artist" + pItem->GetLabel();
    CStdString strDestThumb;
    CUtil::GetCachedThumbnail(strArtist, strDestThumb);
    CPicture picture;
    CFile::Delete(strDestThumb); // remove old thumb
    if (picture.DoCreateThumbnail(strPicture,strDestThumb))
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
  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
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
