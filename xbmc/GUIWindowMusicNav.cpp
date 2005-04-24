#include "stdafx.h"
#include "GUIWindowMusicNav.h"
#include "PlayListFactory.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIPassword.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY   3
#define CONTROL_BTNSORTASC   4

#define CONTROL_LABELFILES         12
#define CONTROL_FILTER    15

#define CONTROL_BTNSHUFFLE   21

#define CONTROL_LIST    50
#define CONTROL_THUMBS    51
#define CONTROL_BIGLIST   52

#define SHOW_ROOT     0
#define SHOW_GENRES     8
#define SHOW_ARTISTS    4
#define SHOW_ALBUMS     2
#define SHOW_SONGS     1

struct SSortMusicNav
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;

    if (rpStart.m_strPath.IsEmpty())
      return true;

    if (rpEnd.m_strPath.IsEmpty())
      return false;

    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;

    if (rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      CStdString strStart;
      CStdString strEnd;
      CStdString strTemp;

      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:   // Sort by Filename
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;

      case 1:   // Sort by Date
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;

        if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
        if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;

        if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
        if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

        if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
        if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

        if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
        if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

        if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
        if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
        return true;
        break;

      case 2:   // Sort by Size
        if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
        if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
        return true;
        break;

      case 3:   // Sort by TrackNum
        if ( rpStart.m_musicInfoTag.GetTrackNumber() > rpEnd.m_musicInfoTag.GetTrackNumber()) return bGreater;
        if ( rpStart.m_musicInfoTag.GetTrackNumber() < rpEnd.m_musicInfoTag.GetTrackNumber()) return !bGreater;
        return true;
        break;

      case 4:   // Sort by Duration
        if ( rpStart.m_musicInfoTag.GetDuration() > rpEnd.m_musicInfoTag.GetDuration()) return bGreater;
        if ( rpStart.m_musicInfoTag.GetDuration() < rpEnd.m_musicInfoTag.GetDuration()) return !bGreater;
        return true;
        break;

      case 5:   // Sort by Title
        // remove "the" for sorting
        strStart = rpStart.m_musicInfoTag.GetTitle();
        strEnd = rpEnd.m_musicInfoTag.GetTitle();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        break;

      case 6:   // Sort by Artist
        // remove "the" for sorting
        strStart = rpStart.m_musicInfoTag.GetArtist();
        strEnd = rpEnd.m_musicInfoTag.GetArtist();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        // concat the album...
        strStart = rpStart.m_musicInfoTag.GetAlbum();
        strEnd = rpEnd.m_musicInfoTag.GetAlbum();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcat(szfilename1, strStart.c_str());
        strcat(szfilename2, strEnd.c_str());
        // and finally the track number
        strStart.Format("%02i", rpStart.m_musicInfoTag.GetTrackNumber());
        strEnd.Format("%02i", rpEnd.m_musicInfoTag.GetTrackNumber());
        strcat(szfilename1, strStart.c_str());
        strcat(szfilename2, strEnd.c_str());
        break;

      case 7:   // Sort by Album
        // remove "the" for sorting
        strStart = rpStart.m_musicInfoTag.GetAlbum();
        strEnd = rpEnd.m_musicInfoTag.GetAlbum();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        // concat the artist (could have multiple Greatest Hits albums for instance)
        strStart = rpStart.m_musicInfoTag.GetArtist();
        strEnd = rpEnd.m_musicInfoTag.GetArtist();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcat(szfilename1, strStart.c_str());
        strcat(szfilename2, strEnd.c_str());
        // and concat the track number
        strStart.Format("%02i", rpStart.m_musicInfoTag.GetTrackNumber());
        strEnd.Format("%02i", rpEnd.m_musicInfoTag.GetTrackNumber());
        strcat(szfilename1, strStart.c_str());
        strcat(szfilename2, strEnd.c_str());
        break;

      case 8:   // Label without "The "
        strStart = rpStart.GetLabel();
        strEnd = rpEnd.GetLabel();
        if (strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        break;

      default:   // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }

      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (int i = 0; i < (int)strlen(szfilename2); i++)
      {
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      }

      if (m_bSortAscending)
        return StringUtils::AlphaNumericCompare(szfilename1, szfilename2);
      else
        return !StringUtils::AlphaNumericCompare(szfilename1, szfilename2);
    }

    if (!rpStart.m_bIsFolder) return false;
    return true;
  }

  static int m_iSortMethod;
  static int m_bSortAscending;
  static CStdString m_strDirectory;
};

int SSortMusicNav::m_iSortMethod;
int SSortMusicNav::m_bSortAscending;
CStdString SSortMusicNav::m_strDirectory;

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
    : CGUIWindowMusicBase()
{
  m_iState = SHOW_ROOT;
  m_iPath = m_iState;
  m_strGenre = "";
  m_strArtist = "";
  m_strAlbum = "";
  m_strAlbumPath = "";
//  m_iThumbControl = CONTROL_THUMBS;
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
      if (m_iViewAsIcons == -1 && m_iViewAsIconsRoot == -1)
      {
        m_iViewAsIcons = g_stSettings.m_iMyMusicNavRootViewAsIcons;
        m_iViewAsIconsRoot = g_stSettings.m_iMyMusicNavRootViewAsIcons;
      }
    }
    break;

  case GUI_MSG_DVDDRIVE_EJECTED_CD:
  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    return true;
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_iState == SHOW_ROOT || m_iState == SHOW_GENRES)
        {
          // sort by label
          // root is not actually sorted though
          g_stSettings.m_iMyMusicNavRootSortMethod = 0;
        }
        else if (m_iState == SHOW_ARTISTS)
        {
          // sort artist names without "the"
          g_stSettings.m_iMyMusicNavRootSortMethod = 8;
        }
        else if (m_iState == SHOW_ALBUMS)
        {
          // allow sort by 6,7
          g_stSettings.m_iMyMusicNavAlbumsSortMethod++;
          if (g_stSettings.m_iMyMusicNavAlbumsSortMethod >= 8) g_stSettings.m_iMyMusicNavAlbumsSortMethod = 6;
        }
        else if (m_iState == SHOW_SONGS)
        {
          // allow sort by 3,4,5,6,7
          g_stSettings.m_iMyMusicNavSongsSortMethod++;
          if (g_stSettings.m_iMyMusicNavSongsSortMethod >= 8) g_stSettings.m_iMyMusicNavSongsSortMethod = 3;
        }
        g_settings.Save();

        int nItem = m_viewControl.GetSelectedItem();
        CFileItem*pItem = m_vecItems[nItem];
        CStdString strSelected = pItem->m_strPath;

        UpdateButtons();
        UpdateListControl();

        CStdString strDirectory = m_Directory.m_strPath;
        if (CUtil::HasSlashAtEnd(strDirectory))
          strDirectory.Delete(strDirectory.size() - 1);

        if (!strDirectory.IsEmpty() && m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory == strDirectory && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC_TEMP)
        {
          int nSong = g_playlistPlayer.GetCurrentSong();
          const CPlayList::CPlayListItem item = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP)[nSong];
          g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
          g_playlistPlayer.Reset();
          int nFolderCount = 0;
          for (int i = 0; i < m_vecItems.Size(); i++)
          {
            CFileItem* pItem = m_vecItems[i];
            if (pItem->m_bIsFolder)
            {
              nFolderCount++;
              continue;
            }
            CPlayList::CPlayListItem playlistItem;
            CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
            g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
            if (item.GetFileName() == pItem->m_strPath)
              g_playlistPlayer.SetCurrentSong(i - nFolderCount);
          }
        }

        for (int i = 0; i < m_vecItems.Size(); i++)
        {
          CFileItem* pItem = m_vecItems[i];
          if (pItem->m_strPath == strSelected)
          {
            m_viewControl.SetSelectedItem(i);
            break;
          }
        }
        return true;
      }
      else if (iControl == CONTROL_BTNVIEWASICONS)
      {
        // First check if we have any additional view ability
        if (m_iState == SHOW_ALBUMS)
        { // allow the extra big list view
          m_iViewAsIcons++;
          if (m_iViewAsIcons > VIEW_AS_LARGE_LIST) m_iViewAsIcons = VIEW_AS_LIST;
          UpdateButtons();
        }
        else
        { // everything else is handled by the base class
          CGUIWindowMusicBase::OnMessage(message);
        }

        if (m_iState == SHOW_ROOT)
          g_stSettings.m_iMyMusicNavRootViewAsIcons = m_iViewAsIconsRoot;
        else if (m_iState == SHOW_GENRES)
          g_stSettings.m_iMyMusicNavGenresViewAsIcons = m_iViewAsIcons;
        else if (m_iState == SHOW_ARTISTS)
          g_stSettings.m_iMyMusicNavArtistsViewAsIcons = m_iViewAsIcons;
        else if (m_iState == SHOW_ALBUMS)
          g_stSettings.m_iMyMusicNavAlbumsViewAsIcons = m_iViewAsIcons;
        else if (m_iState == SHOW_SONGS)
          g_stSettings.m_iMyMusicNavSongsViewAsIcons = m_iViewAsIcons;

        g_settings.Save();

        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_iState == SHOW_ROOT)
          return true;
        else if (m_iState == SHOW_GENRES)
          g_stSettings.m_bMyMusicNavGenresSortAscending = !g_stSettings.m_bMyMusicNavGenresSortAscending;
        else if (m_iState == SHOW_ARTISTS)
          g_stSettings.m_bMyMusicNavArtistsSortAscending = !g_stSettings.m_bMyMusicNavArtistsSortAscending;
        else if (m_iState == SHOW_ALBUMS)
          g_stSettings.m_bMyMusicNavAlbumsSortAscending = !g_stSettings.m_bMyMusicNavAlbumsSortAscending;
        else if (m_iState == SHOW_SONGS)
          g_stSettings.m_bMyMusicNavSongsSortAscending = !g_stSettings.m_bMyMusicNavSongsSortAscending;
        g_settings.Save();

        UpdateButtons();
        UpdateListControl();

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
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused()) return true;
            if (g_application.GetPlaySpeed() != 1) return true;
          }

          // not playing audio, or playback speed == 1
          PlayItem(iItem);
        }
      }
    }
    break;
  }
  return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CGUIWindowMusicNav::GetDirectory(%s)",strDirectory.c_str());
  CLog::Log(LOGDEBUG, "  strGenre = [%s], strArtist = [%s], strAlbum = [%s], strAlbumPath = [%s]",m_strGenre.c_str(), m_strArtist.c_str(), m_strAlbum.c_str(), m_strAlbumPath.c_str());

  // cleanup items
  if (items.Size())
  {
    items.Clear();
  }

  switch (m_iState)
  {
  case SHOW_ROOT:
    {
      g_stSettings.m_iMyMusicNavRootSortMethod = 0;
      m_iViewAsIconsRoot = g_stSettings.m_iMyMusicNavRootViewAsIcons;

      // we're at the zero point
      // add the initial items to the fileitems
      vector<CStdString> vecRoot;
      vecRoot.push_back(g_localizeStrings.Get(135)); // Genres
      vecRoot.push_back(g_localizeStrings.Get(133));  // Artists
      vecRoot.push_back(g_localizeStrings.Get(132));  // Albums
      vecRoot.push_back(g_localizeStrings.Get(134));  // Songs
      for (int i = 0; i < (int)vecRoot.size();++i)
      {
        CFileItem* pFileItem = new CFileItem(vecRoot[i]);
        pFileItem->m_strPath = vecRoot[i];
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);
      }
    }
    break;

  case SHOW_GENRES:
    {
      g_stSettings.m_iMyMusicNavRootSortMethod = 0;
      m_iViewAsIcons = g_stSettings.m_iMyMusicNavGenresViewAsIcons;

      // set parent directory
      if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
      {
        CFileItem* pFileItem = new CFileItem("..");
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);
      }

      // get genres from the database
      VECGENRES genres;
      bool bTest = g_musicDatabase.GetGenresNav(genres);

      // Display an error message if the database doesn't contain any genres
      DisplayEmptyDatabaseMessage(genres.empty());

      if (bTest)
      {
        // add "All Genres"
        CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15105));
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);

        for (int i = 0; i < (int)genres.size(); ++i)
        {
          CStdString strGenre = genres[i];
          CFileItem* pFileItem = new CFileItem(strGenre);
          pFileItem->m_strPath = strGenre;
          pFileItem->m_bIsFolder = true;
          items.Add(pFileItem);
        }
      }
    }
    break;

  case SHOW_ARTISTS:
    {
      g_stSettings.m_iMyMusicNavRootSortMethod = 8;
      m_iViewAsIcons = g_stSettings.m_iMyMusicNavArtistsViewAsIcons;

      // set parent directory
      if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
      {
        CFileItem* pFileItem = new CFileItem("..");
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);
      }

      // get artists from the database
      VECARTISTS artists;
      bool bTest = g_musicDatabase.GetArtistsNav(artists, m_strGenre);

      // Display an error message if the database doesn't contain any artists
      DisplayEmptyDatabaseMessage(artists.empty());

      if (bTest)
      {
        // add "All Artists"
        CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15103));
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);

        for (int i = 0; i < (int)artists.size(); ++i)
        {
          CStdString strArtist = artists[i];
          CFileItem* pFileItem = new CFileItem(strArtist);
          pFileItem->m_strPath = strArtist;
          pFileItem->m_bIsFolder = true;
          items.Add(pFileItem);
        }
      }
    }
    break;

  case SHOW_ALBUMS:
    {
      m_iViewAsIcons = g_stSettings.m_iMyMusicNavAlbumsViewAsIcons;

      // set parent directory
      if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
      {
        CFileItem* pFileItem = new CFileItem("..");
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);
      }

      // get albums from the database
      VECALBUMS albums;
      bool bTest = g_musicDatabase.GetAlbumsNav(albums, m_strGenre, m_strArtist);

      // Display an error message if the database doesn't contain any albums
      DisplayEmptyDatabaseMessage(albums.empty());

      if (bTest)
      {
        // add "All Albums"
        CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15102));
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);

        for (int i = 0; i < (int)albums.size(); ++i)
        {
          CAlbum &album = albums[i];
          CFileItem* pFileItem = new CFileItem(album);
          items.Add(pFileItem);
        }
      }
    }
    break;

  case SHOW_SONGS:
    {
      m_iViewAsIcons = g_stSettings.m_iMyMusicNavSongsViewAsIcons;

      // set parent directory
      if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
      {
        CFileItem* pFileItem = new CFileItem("..");
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);
      }

      // get songs from the database
      VECSONGS songs;
      bool bTest = g_musicDatabase.GetSongsNav(songs, m_strGenre, m_strArtist, m_strAlbum, m_strAlbumPath);

      // Display an error message if the database doesn't contain any albums
      DisplayEmptyDatabaseMessage(songs.empty());

      if (bTest)
      {
        // add "All Songs"
        CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15104));
        pFileItem->m_strPath = "";
        pFileItem->m_bIsFolder = true;
        items.Add(pFileItem);

        for (int i = 0; i < (int)songs.size(); ++i)
        {
          CSong &song = songs[i];
          CFileItem* pFileItem = new CFileItem(song);
          items.Add(pFileItem);
        }
      }
    }
    break;
  }
}

void CGUIWindowMusicNav::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // disallow sorting on the root
  if (m_iState == SHOW_ROOT)
  {
    CONTROL_DISABLE(CONTROL_BTNSORTBY);
    CONTROL_DISABLE(CONTROL_BTNSORTASC);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSORTBY);
    CONTROL_ENABLE(CONTROL_BTNSORTASC);
  }

  // Update sorting control
  bool bSortAscending = false;
  if (m_iState == SHOW_ROOT)
    bSortAscending = false;
  else if (m_iState == SHOW_GENRES)
    bSortAscending = g_stSettings.m_bMyMusicNavGenresSortAscending;
  else if (m_iState == SHOW_ARTISTS)
    bSortAscending = g_stSettings.m_bMyMusicNavArtistsSortAscending;
  else if (m_iState == SHOW_ALBUMS)
    bSortAscending = g_stSettings.m_bMyMusicNavAlbumsSortAscending;
  else if (m_iState == SHOW_SONGS)
    bSortAscending = g_stSettings.m_bMyMusicNavSongsSortAscending;

  if (bSortAscending)
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

  if (m_iState == SHOW_ROOT)
    m_viewControl.SetCurrentView(m_iViewAsIconsRoot);
  else
    m_viewControl.SetCurrentView(m_iViewAsIcons);

  // Update object count
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);

  // Update sort by button
  if (m_iState == SHOW_ROOT || m_iState == SHOW_GENRES || m_iState == SHOW_ARTISTS)
  {
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, 103);
  }
  else if (m_iState == SHOW_ALBUMS)
  {
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, g_stSettings.m_iMyMusicNavAlbumsSortMethod + 263);
  }
  else if (m_iState == SHOW_SONGS)
  {
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, g_stSettings.m_iMyMusicNavSongsSortMethod + 263);
  }

  // make the filter label
  CStdString strLabel = m_strGenre;

  // Append Artist
  if (!strLabel.IsEmpty() && !m_strArtist.IsEmpty())
    strLabel += "/";
  if (!m_strArtist.IsEmpty())
    strLabel += m_strArtist;

  // Append Album
  if (!strLabel.IsEmpty() && !m_strAlbum.IsEmpty())
    strLabel += "/";
  if (!m_strAlbum.IsEmpty())
    strLabel += m_strAlbum;

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  // Mark the shuffle button
  if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP))
  {
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);
  }
}

void CGUIWindowMusicNav::OnClick(int iItem)
{
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  CStdString strNextPath = m_Directory.m_strPath;
  if (strNextPath.IsEmpty())
    strNextPath = "db://";
  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "music" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    if (pItem->GetLabel() == "..")
    {
      // go back a directory
      GoParentFolder();

      // GoParentFolder() calls Update(), so just return
      return ;
    }
    else
    {
      switch (m_iState)
      {
        // set state to the new directory
      case SHOW_ROOT:
        {
          // genres
          if (strPath.Equals(CStdString(g_localizeStrings.Get(135).c_str())))
            m_iState = SHOW_GENRES;
          // artists
          else if (strPath.Equals(CStdString(g_localizeStrings.Get(133).c_str())))
            m_iState = SHOW_ARTISTS;
          // albums
          else if (strPath.Equals(CStdString(g_localizeStrings.Get(132).c_str())))
            m_iState = SHOW_ALBUMS;
          else
            m_iState = SHOW_SONGS;
          m_iPath += m_iState;
        }
        break;

      case SHOW_GENRES:
        {
          m_iState = SHOW_ARTISTS;
          m_iPath += m_iState;
          m_strGenre = strPath;

          // clicked on "All Genres" ?
          if (strPath.IsEmpty())
            m_strGenre.Empty();
        }
        break;

      case SHOW_ARTISTS:
        {
          m_iState = SHOW_ALBUMS;
          m_iPath += m_iState;
          m_strArtist = strPath;

          // clicked on "All Artists" ?
          if (strPath.IsEmpty())
            m_strArtist.Empty();
        }
        break;

      case SHOW_ALBUMS:
        {
          m_iState = SHOW_SONGS;
          m_iPath += m_iState;
          m_strAlbum = pItem->m_musicInfoTag.GetAlbum();
          m_strAlbumPath = pItem->m_strPath;

          // clicked on "All Albums" ?
          if (strPath.IsEmpty())
          {
            m_strAlbum.Empty();
            m_strAlbumPath.Empty();
          }

        }
        break;
      }
    }
    strNextPath += pItem->GetLabel() + "/";
    Update(strNextPath);
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
    m_strTempPlayListDirectory = m_Directory.m_strPath;
    if (CUtil::HasSlashAtEnd(m_strTempPlayListDirectory))
      m_strTempPlayListDirectory.Delete(m_strTempPlayListDirectory.size() - 1);

    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
    g_playlistPlayer.Play(iItem - nFolderCount);
  }
}

void CGUIWindowMusicNav::OnFileItemFormatLabel(CFileItem* pItem)
{
  // clear label for special directories
  if (pItem->m_strPath.IsEmpty())
    pItem->SetLabel2("");
  else if (pItem->m_bIsFolder)
  {
    // for albums, set label2 to the artist name
    if (m_iState == SHOW_ALBUMS)
    {
      // if filtering use the filtered artist name
      if (!m_strArtist.IsEmpty())
        pItem->SetLabel2(m_strArtist);
      else
      {
        // otherwise use the label from the album
        // this may be "Various Artists"
        CStdString strArtist = pItem->m_musicInfoTag.GetArtist();
        pItem->SetLabel2(strArtist);
      }
    }
    // for root, genres, artists, clear label2
    else
      pItem->SetLabel2("");
  }
  else
  {
    // for songs, set the label using user defined format string
    if (pItem->m_musicInfoTag.Loaded())
      SetLabelFromTag(pItem);
  }

  // set thumbs and default icons
  if (m_iState != SHOW_ROOT)
    pItem->SetMusicThumb();
  if (pItem->GetIconImage() == "music.jpg")
    pItem->SetThumbnailImage("MyMusic.jpg");
  pItem->FillInDefaultIcon();
}

void CGUIWindowMusicNav::DoSort(CFileItemList& items)
{
  // dont sort the root window
  if (m_iState == SHOW_ROOT)
    return ;

  SSortMusicNav::m_strDirectory = m_Directory.m_strPath;

  if (m_iState == SHOW_GENRES)
  {
    SSortMusicNav::m_iSortMethod = g_stSettings.m_iMyMusicNavRootSortMethod;
    SSortMusicNav::m_bSortAscending = g_stSettings.m_bMyMusicNavGenresSortAscending;
  }
  else if (m_iState == SHOW_ARTISTS)
  {
    SSortMusicNav::m_iSortMethod = g_stSettings.m_iMyMusicNavRootSortMethod;
    SSortMusicNav::m_bSortAscending = g_stSettings.m_bMyMusicNavArtistsSortAscending;
  }
  else if (m_iState == SHOW_ALBUMS)
  {
    SSortMusicNav::m_iSortMethod = g_stSettings.m_iMyMusicNavAlbumsSortMethod;
    SSortMusicNav::m_bSortAscending = g_stSettings.m_bMyMusicNavAlbumsSortAscending;
  }
  else if (m_iState == SHOW_SONGS)
  {
    SSortMusicNav::m_iSortMethod = g_stSettings.m_iMyMusicNavSongsSortMethod;
    SSortMusicNav::m_bSortAscending = g_stSettings.m_bMyMusicNavSongsSortAscending;
  }

  items.Sort(SSortMusicNav::Sort);
}

void CGUIWindowMusicNav::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    if (pSelItem->m_musicInfoTag.GetAlbum().IsEmpty())
    {
      m_iState = SHOW_ARTISTS;
      m_iPath = m_iState;
      m_strGenre.Empty();
      m_strAlbum.Empty();
      m_strArtist.Empty();
      CStdString strPath = "db://Search/Artist/";
      Update(strPath);
      for (int i = 0; i < (int)m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_strPath == pSelItem->m_strPath)
        {
          m_viewControl.SetSelectedItem(i);
          m_viewControl.SetFocused();
          break;
        }
      }
    }
    else
    {
      // FIXME: various artist albums are not handled
      m_iState = SHOW_ALBUMS;
      m_iPath = m_iState;

      m_strGenre.Empty();
      m_strArtist = pSelItem->m_musicInfoTag.GetArtist();
      m_strAlbum.Empty();

      CStdString strPath = "db://Search/Albums/" + m_strArtist;
      Update(strPath);

      CStdString strHistory;
      GetDirectoryHistoryString(pSelItem, strHistory);
      m_history.Set(strHistory, strPath);
      m_history.Set(strPath, "");

      for (int i = 0; i < (int)m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_strPath == pSelItem->m_strPath)
        {
          m_viewControl.SetSelectedItem(i);
          m_viewControl.SetFocused();
          break;
        }
      }
    }
  }
  else
  {
    CStdString strPath;
    CUtil::GetDirectory(pSelItem->m_strPath, strPath);

    m_strGenre.Empty();
    m_strArtist = pSelItem->m_musicInfoTag.GetArtist();
    m_strAlbum = pSelItem->m_musicInfoTag.GetAlbum();

    m_iState = SHOW_SONGS;
    m_iPath = m_iState;

    Update(strPath);

    CFileItem parentItem(*pSelItem);
    parentItem.m_bIsFolder = true;
    parentItem.m_strPath = strPath;

    CStdString strHistory;
    GetDirectoryHistoryString(&parentItem, strHistory);
    m_history.Set(strHistory, m_strArtist);

    m_history.Set(m_strArtist, "");

    for (int i = 0; i < (int)m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_strPath == pSelItem->m_strPath)
      {
        m_viewControl.SetSelectedItem(i);
        m_viewControl.SetFocused();
        break;
      }
    }
  }
}

/// \brief Search for songs, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  VECARTISTS artists;
  g_musicDatabase.GetArtistsByName(strSearch, artists);

  if (artists.size())
  {
    CStdString strSong = g_localizeStrings.Get(484); // Artist
    for (int i = 0; i < (int)artists.size(); i++)
    {
      CStdString& strArtist = artists[i];
      CFileItem* pItem = new CFileItem(strArtist);
      pItem->m_strPath = strArtist;
      pItem->m_bIsFolder = true;
      pItem->SetLabel("[" + strSong + "] " + strArtist);
      items.Add(pItem);
    }
  }

  VECALBUMS albums;
  g_musicDatabase.FindAlbumsByName(strSearch, albums);

  if (albums.size())
  {
    CStdString strAlbum = g_localizeStrings.Get(483); // Album
    for (int i = 0; i < (int)albums.size(); i++)
    {
      CAlbum& album = albums[i];
      CFileItem* pItem = new CFileItem(album);
      pItem->SetLabel("[" + strAlbum + "] " + album.strAlbum + " - " + album.strArtist);
      items.Add(pItem);
    }
  }

  VECSONGS songs;
  g_musicDatabase.FindSongsByName(strSearch, songs);

  if (songs.size())
  {
    CStdString strSong = g_localizeStrings.Get(179); // Song
    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong& song = songs[i];
      CFileItem* pItem = new CFileItem(song);
      pItem->SetLabel("[" + strSong + "] " + song.strTitle + " - " + song.strArtist + " - " + song.strAlbum);
      items.Add(pItem);
    }
  }
}

/// \brief Call to go to parent folder
void CGUIWindowMusicNav::GoParentFolder()
{
  // go back a directory
  m_iPath -= m_iState;

  // do we go back to albums?
  if (m_iPath & (1 << 1))
  {
    m_iState = SHOW_ALBUMS;
    m_strAlbum.Empty();
    m_strAlbumPath.Empty();
  }
  // or back to artists?
  else if (m_iPath & (1 << 2))
  {
    m_iState = SHOW_ARTISTS;
    m_strAlbum.Empty();
    m_strAlbumPath.Empty();
    m_strArtist.Empty();
  }
  // or back to genres?
  else if (m_iPath & (1 << 3))
  {
    m_iState = SHOW_GENRES;
    m_strAlbum.Empty();
    m_strAlbumPath.Empty();
    m_strArtist.Empty();
    m_strGenre.Empty();
  }
  // or back to the root?
  else
  {
    m_iState = SHOW_ROOT;
    m_strAlbum.Empty();
    m_strAlbumPath.Empty();
    m_strArtist.Empty();
    m_strGenre.Empty();
  }

  CStdString strOldPath = m_Directory.m_strPath;
  CUtil::GetParentPath(strOldPath, m_strParentPath);

  // parent path??
  CLog::Log(LOGDEBUG, "CGUIWindowMusicNav::GoParentFolder(%s), m_strParentPath = [%s]", strOldPath.c_str(), m_strParentPath.c_str());

  Update(m_strParentPath);

  if (!g_guiSettings.GetBool("FileLists.FullDirectoryHistory"))
    m_history.Remove(strOldPath); // delete current path
}

void CGUIWindowMusicNav::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  //strHistoryString=pItem->m_strPath;
  strHistoryString = m_Directory.m_strPath;
  if (!CUtil::HasSlashAtEnd(strHistoryString))
    strHistoryString += "/";
  strHistoryString += pItem->GetLabel();
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicNav::AddItemToPlayList(const CFileItem* pItem)
{
  // cant do it from the root
  if (m_iState == SHOW_ROOT) return ;
  if (pItem->m_bIsFolder)
  {
    // skip ".."
    if (pItem->GetLabel() == "..") return ;

    // save state
    int iOldState = m_iState;
    int iOldPath = m_iPath;
    CStdString strOldGenre = m_strGenre;
    CStdString strOldArtist = m_strArtist;
    CStdString strOldAlbum = m_strAlbum;
    CStdString strOldAlbumPath = m_strAlbumPath;
    CStdString strDirectory = m_Directory.m_strPath;
    CStdString strOldParentPath = m_strParentPath;
    int iViewAsIcons = m_iViewAsIcons;

    // update filter with currently selected item
    switch (m_iState)
    {
    case SHOW_GENRES:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strGenre.Empty();
        else
          m_strGenre = pItem->m_strPath;
      }
      break;

    case SHOW_ARTISTS:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strArtist.Empty();
        else
          m_strArtist = pItem->m_strPath;
      }
      break;

    case SHOW_ALBUMS:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strAlbum.Empty();
        else
        {
          m_strAlbum = pItem->m_musicInfoTag.GetAlbum();
          m_strAlbumPath = pItem->m_strPath;
        }
      }
      break;
    }

    m_iState = SHOW_SONGS;
    CFileItemList items;
    GetDirectory("Playlist", items);
    DoSort(items);
    for (int i = 0; i < items.Size(); ++i)
    {
      if (!items[i]->m_strPath.IsEmpty())
        AddItemToPlayList(items[i]);
    }

    // restore old state
    m_iState = iOldState;
    m_iPath = iOldPath;
    m_strGenre = strOldGenre;
    m_strArtist = strOldArtist;
    m_strAlbum = strOldAlbum;
    m_strAlbumPath = strOldAlbumPath;
    m_Directory.m_strPath = strDirectory;
    m_strParentPath = strOldParentPath;
    m_iViewAsIcons = iViewAsIcons;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
    }
  }
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicNav::AddItemToTempPlayList(const CFileItem* pItem)
{
  if (pItem->m_bIsFolder)
  {
    // save state
    int iOldState = m_iState;
    int iOldPath = m_iPath;
    CStdString strOldGenre = m_strGenre;
    CStdString strOldArtist = m_strArtist;
    CStdString strOldAlbum = m_strAlbum;
    CStdString strOldAlbumPath = m_strAlbumPath;
    CStdString strDirectory = m_Directory.m_strPath;
    CStdString strOldParentPath = m_strParentPath;
    int iViewAsIcons = m_iViewAsIcons;

    // update filter with currently selected item
    switch (m_iState)
    {
    case SHOW_GENRES:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strGenre.Empty();
        else
          m_strGenre = pItem->m_strPath;
      }
      break;

    case SHOW_ARTISTS:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strArtist.Empty();
        else
          m_strArtist = pItem->m_strPath;
      }
      break;

    case SHOW_ALBUMS:
      {
        if (pItem->m_strPath.IsEmpty())
          m_strAlbum.Empty();
        else
        {
          m_strAlbum = pItem->m_musicInfoTag.GetAlbum();
          m_strAlbumPath = pItem->m_strPath;
        }
      }
      break;
    }

    m_iState = SHOW_SONGS;
    CFileItemList items;
    GetDirectory("Playlist", items);
    DoSort(items);
    for (int i = 0; i < items.Size(); ++i)
    {
      if (!items[i]->m_strPath.IsEmpty())
        AddItemToTempPlayList(items[i]);
    }

    // restore old state
    m_iState = iOldState;
    m_iPath = iOldPath;
    m_strGenre = strOldGenre;
    m_strArtist = strOldArtist;
    m_strAlbum = strOldAlbum;
    m_strAlbumPath = strOldAlbumPath;
    m_Directory.m_strPath = strDirectory;
    m_strParentPath = strOldParentPath;
    m_iViewAsIcons = iViewAsIcons;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
    }
  }
}


void CGUIWindowMusicNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_iState == SHOW_ROOT)
    return;

  CGUIWindowMusicBase::PlayItem(iItem);
}
