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
#include "GUIWindowMusicSongs.h"
#include "Picture.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "GUIViewState.h"

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
  m_bGotDirFromCache = false;
  m_SortCache = SORT_METHOD_NONE;
  m_AscendCache = SORT_ORDER_NONE;
  m_bSkipTheCache = false;
  m_bDisplayEmptyDatabaseMessage=false;

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
          share.m_strThumbnailImage="defaultFolder.png";
          m_shares.push_back(share);
        }

        //  Playlists share
        CShare share;
        share.strName=g_localizeStrings.Get(136); // Playlists
        share.strPath=CUtil::MusicPlaylistsLocation();
        share.m_strThumbnailImage="defaultFolder.png";
        m_shares.push_back(share);

        // setup shares and file filters
        m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
        m_rootDir.SetShares(m_shares);

        m_vecItems.m_strPath = "";
        m_bSkipTheCache = g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting");
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

      DisplayEmptyDatabaseMessage(g_musicDatabase.GetSongsCount() > 0);

      CGUIWindowMusicBase::OnMessage(message);

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
      if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        bool bGotDirFromCache=m_bGotDirFromCache;
        m_bGotDirFromCache=false;

        CGUIWindowMusicBase::OnMessage(message);

        m_bGotDirFromCache=bGotDirFromCache;

        return true;
      }
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        bool bGotDirFromCache=m_bGotDirFromCache;
        m_bGotDirFromCache=false;

        CGUIWindowMusicBase::OnMessage(message);

        m_bGotDirFromCache=bGotDirFromCache;

        return true;
      }
      else if (iControl == CONTROL_BTNSHUFFLE) // shuffle?
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

  m_bGotDirFromCache=false;

  CFileItem directory(strDirectory, true);
  if (directory.IsPlayList())
  {
    return GetSongsFromPlayList(strDirectory, items);
  }
  else if (CanCache(strDirectory))
  {
    m_bGotDirFromCache=LoadDatabaseDirectoryCache(strDirectory, items, m_SortCache, m_AscendCache, m_bSkipTheCache);
    if (m_bGotDirFromCache)
      return true;
  }

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
  //CStdString strLabel;

  //// "Top 100 Songs"
  //if (m_iPath == SHOW_TOP)
  //  strLabel = g_localizeStrings.Get(271);
  //// "Top 100 Songs"
  //else if (m_iPath == (SHOW_TOP + SHOW_SONGS))
  //  strLabel = g_localizeStrings.Get(10504);
  //// "Top 100 Albums"
  //else if (m_iPath == (SHOW_TOP + SHOW_ALBUMS))
  //  strLabel = g_localizeStrings.Get(10505);
  //// "Recently Added Albums"
  //else if (m_iPath == SHOW_RECENTLY_ADDED)
  //  strLabel = g_localizeStrings.Get(359);
  //// "Recently Played Albums"
  //else if (m_iPath == SHOW_RECENTLY_PLAYED)
  //  strLabel = g_localizeStrings.Get(517);
  //// "Playlists"
  //else if (m_iPath == SHOW_PLAYLISTS)
  //  strLabel = g_localizeStrings.Get(136);
  //// Playlist name
  //else if (m_iPath == SHOW_PLAYLISTS + SHOW_SONGS)
  //  strLabel = CUtil::GetFileName(m_vecPathHistory.back());
  //// "Genre/Artist/Album"
  //else
  //{
  //  strLabel = m_strGenre;

  //  // Append Artist
  //  if (!strLabel.IsEmpty() && !m_strArtist.IsEmpty())
  //    strLabel += "/";
  //  if (!m_strArtist.IsEmpty())
  //    strLabel += m_strArtist;

  //  // Append Album
  //  if (!strLabel.IsEmpty() && !m_strAlbum.IsEmpty())
  //    strLabel += "/";
  //  if (!m_strAlbum.IsEmpty())
  //    strLabel += m_strAlbum;
  //}

  //SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  // Mark the shuffle button
  if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP))
  {
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);
  }
}

void CGUIWindowMusicNav::OnClick(int iItem)
{
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strLabel = pItem->GetLabel();

  CStdString strPath = pItem->m_strPath;
  CStdString strNextPath = m_vecItems.m_strPath;
  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "music" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    if (pItem->IsParentFolder())
    {
      // go back a directory
      GoParentFolder();

      // GoParentFolder() calls Update(), so just return
      return ;
    }
    else
    {
      Update(strPath);
    }
  }
  else
  {
    //  treat playlists like folders
    if (pItem->IsPlayList())
    {
      m_vecPathHistory.push_back(strPath);
      Update(strPath);
    }
    else
    {
      // play and add current directory to temporary playlist
      int nFolderCount = 0;
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
      g_playlistPlayer.Reset();
      for ( int i = 0; i < m_vecItems.Size(); i++ )
      {
        CFileItem* pItem = m_vecItems[i];
        if ( pItem->m_bIsFolder )
        {
          nFolderCount++;
          continue;
        }
        CPlayList::CPlayListItem playlistItem;
        CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
        g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
      }

      // Save current window and directory to know where the selected item was
      m_nTempPlayListWindow = GetID();
      m_strTempPlayListDirectory = m_vecItems.m_strPath;
      if (CUtil::HasSlashAtEnd(m_strTempPlayListDirectory))
        m_strTempPlayListDirectory.Delete(m_strTempPlayListDirectory.size() - 1);

      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
      g_playlistPlayer.Play(iItem - nFolderCount);
    }
  }
}

void CGUIWindowMusicNav::OnFileItemFormatLabel(CFileItem* pItem)
{
  // skip if directory was returned from cache
  if (m_bGotDirFromCache) return;

  //  Folders have a preformated label
  if (!pItem->m_bIsFolder)
    SetLabelFromTag(pItem);

  if (pItem->GetIconImage() == "defaultAlbumCover.png")
    pItem->SetThumbnailImage("defaultAlbumCover.png");

  pItem->FillInDefaultIcon();
}

void CGUIWindowMusicNav::SortItems(CFileItemList& items)
{
  auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), items));
  if (pState.get())
  {
    SORT_METHOD sortMethod=pState->GetSortMethod();
    SORT_ORDER sortAscending=pState->GetSortOrder();
    bool bSortingCurrentDir=items.m_strPath==m_vecItems.m_strPath;
    // skip if directory was returned from cache and 
    // the items we are now sorting are the current ones
    // and the sort parameters match
    if (
      (bSortingCurrentDir) && 
      (m_bGotDirFromCache) && 
      (m_SortCache == sortMethod) &&
      (m_AscendCache == sortAscending) &&
      (m_bSkipTheCache == g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
      )
      return;

    // else sort the list, and the save it to cache
    CGUIWindowMusicBase::SortItems(items);

    if (bSortingCurrentDir && CanCache(items.m_strPath))
      SaveDatabaseDirectoryCache(items.m_strPath, items, sortMethod, sortAscending, g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"));
  }

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
  int btn_Info = pMenu->AddButton(13351);     // Music Information
  int btn_Queue = pMenu->AddButton(13347);    // Queue Item
  //int btn_Play = pMenu->AddButton(13358);   // Play Item
  int btn_PlayWith = pMenu->AddButton(15213); // Play using alternate player
  int btn_NowPlay = pMenu->AddButton(13350);  // Now Playing...
  int btn_Search = pMenu->AddButton(137);     // Search...
  int btn_Thumb = pMenu->AddButton(13359);    // Set Artist Thumb  
  int btn_Settings = pMenu->AddButton(5);     // Settings...

  // check what players we have, if we have multiple display play with option
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

  // turn off info/queue/play/set artist thumb if the current item is goto parent ..
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
  if (bIsGotoParent)
  {
    pMenu->EnableButton(btn_Info, false);
    pMenu->EnableButton(btn_Queue, false);
    //pMenu->EnableButton(btn_Play, false);
    pMenu->EnableButton(btn_PlayWith, false );
    pMenu->EnableButton(btn_Settings, false);
  }
  else
  {
    // only enable play using if we have more than one player available
    pMenu->EnableButton(btn_PlayWith, vecCores.size() >= 1 );
  }

  // turn off the music info button on non-album-able items
  if (!HasAlbumInfo(m_vecItems[iItem]->m_strPath))
    pMenu->EnableButton(btn_Info, false);

  // turn off the now playing button if playlist is empty
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() <= 0)
    pMenu->EnableButton(btn_NowPlay, false);

  // turn off set artist image if not at artist listing.
  // (uses file browser to pick an image)
  if (!IsArtistDir(m_vecItems[iItem]->m_strPath))
    pMenu->EnableButton(6, false);

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());

  int btnid = pMenu->GetButton();
  if( btnid  == btn_Info ) // Music Information
  {
    OnInfo(iItem);
  }
  else if( btnid  == btn_Queue )  // Queue Item
  {
    OnQueueItem(iItem);
  }
  //else if( btnid == btn_Play )
  //{
  //  PlayItem(iItem);
  //}
  else if( btnid  == btn_NowPlay )  // Now Playing...
  {
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return;
  }
  else if( btnid  == btn_Search )  // Search
  {
    OnSearch();
  }
  else if( btnid  == btn_Thumb )  // Set Artist Image
  {
    SetArtistImage(iItem);
  }
  else if( btnid  == btn_PlayWith )
  {
    g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, iPosX, iPosY);
    if( g_application.m_eForcedNextPlayer != EPC_NONE )
      PlayItem(iItem);
  }
  else if( btnid  == btn_Settings )  // Settings
  {
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
    return;
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
      ClearDatabaseDirectoryCache(m_vecItems.m_strPath);    
      Update(m_vecItems.m_strPath);
    }
    else
      CLog::Log(LOGERROR,"  Could not cache artist thumb: %s",strPicture.c_str());
  }
}

bool CGUIWindowMusicNav::IsArtistDir(const CStdString& strDirectory)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE node=dir.GetDirectoryType(strDirectory);
  return (node==NODE_TYPE_ARTIST);
}

bool CGUIWindowMusicNav::CanCache(const CStdString& strDirectory)
{
  //  Only cache the directorys shown in the root of the window
  CMusicDatabaseDirectory dir;
  NODE_TYPE node=dir.GetDirectoryChildType(strDirectory);
  NODE_TYPE parentNode=dir.GetDirectoryType(strDirectory);
  return ((node==NODE_TYPE_GENRE || node==NODE_TYPE_ARTIST || 
          node==NODE_TYPE_ALBUM || node==NODE_TYPE_SONG) &&
          parentNode==NODE_TYPE_OVERVIEW);
}

bool CGUIWindowMusicNav::HasAlbumInfo(const CStdString& strDirectory)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE node=dir.GetDirectoryType(strDirectory);
  return (node!=NODE_TYPE_OVERVIEW && node!=NODE_TYPE_TOP100 && 
          node!=NODE_TYPE_GENRE && node!=NODE_TYPE_ARTIST);
}

void CGUIWindowMusicNav::ClearDatabaseDirectoryCache(const CStdString& strDirectory)
{
  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  strFileName.Format("Z:\\db-%08x.fi", crc);
  CFile::Delete(strFileName);
}

void CGUIWindowMusicNav::SaveDatabaseDirectoryCache(const CStdString& strDirectory, CFileItemList& items, SORT_METHOD SortMethod, SORT_ORDER Ascending, bool bSkipThe)
{
  int iSize = items.Size();
  if (iSize <= 0)
    return;

  CLog::Log(LOGDEBUG,"Caching database directory [%s]",strDirectory.c_str());

  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  strFileName.Format("Z:\\db-%08x.fi", crc);

  CFile file;
  if (file.OpenForWrite(strFileName, true, true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << SortMethod;
    ar << Ascending;
    ar << bSkipThe;
    ar << items;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s, skipthe: %s",iSize,SortMethod,Ascending ? "true" : "false",bSkipThe ? "true" : "false");
    ar.Close();
    file.Close();
  }
}

bool CGUIWindowMusicNav::LoadDatabaseDirectoryCache(const CStdString& strDirectory, CFileItemList& items, SORT_METHOD& SortMethod, SORT_ORDER& Ascending, bool& bSkipThe)
{
  CLog::Log(LOGDEBUG,"Loading database directory [%s]",strDirectory.c_str());

  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  strFileName.Format("Z:\\db-%08x.fi", crc);

  CFile file;
  if (file.Open(strFileName))
  {
    CArchive ar(&file, CArchive::load);
    ar >> (int&)SortMethod;
    ar >> (int&)Ascending;
    ar >> bSkipThe;
    ar >> items;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s, skipthe: %s",items.Size(),SortMethod,Ascending ? "true" : "false",bSkipThe ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

bool CGUIWindowMusicNav::GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items)
{
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

    items.m_strPath=strPlayList;
  }

  return true;
}
