#include "stdafx.h"
#include "GUIViewStateMusic.h"
#include "AutoSwitch.h"
#include "playlistplayer.h"

#include "filesystem/musicdatabasedirectory.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CGUIViewStateWindowMusic::CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items)
{
  if (!items.IsVirtualDirectoryRoot())
    m_hideParentDirItems = g_guiSettings.GetBool("MyMusic.HideParentDirItems");
}

int CGUIViewStateWindowMusic::GetPlaylist()
{
  return PLAYLIST_MUSIC_TEMP;
}

bool CGUIViewStateWindowMusic::HandleArchives()
{
  return g_guiSettings.GetBool("MusicFiles.HandleArchives");
}

bool CGUIViewStateWindowMusic::AutoPlayNextItem()
{
  return g_guiSettings.GetBool("MusicFiles.AutoPlayNextItem");
}

CStdString CGUIViewStateWindowMusic::GetLockType()
{
  return "music";
}

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(items.m_strPath);
  NODE_TYPE ParentNodeType=dir.GetDirectoryType(items.m_strPath);

  CStdString strTrackLeft=g_guiSettings.GetString("MyMusic.TrackFormat");
  CStdString strTrackRight=g_guiSettings.GetString("MyMusic.TrackFormatRight");

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%F", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SORT_METHOD_NONE);

      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavRootViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%F", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SORT_METHOD_NONE);

      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavTopViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_GENRE:
    {
      AddSortMethod(SORT_METHOD_GENRE, 103, LABEL_MASKS("%F", "", "%G", ""));  // Filename, empty | Genre, empty
      SetSortMethod(SORT_METHOD_GENRE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavGenresViewMethod);

      SetSortOrder(g_stSettings.m_MyMusicNavGenresSortOrder);
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
      {
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 103, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
        SetSortMethod(SORT_METHOD_ARTIST_IGNORE_THE);
      }
      else
      {
        AddSortMethod(SORT_METHOD_ARTIST, 103, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
        SetSortMethod(SORT_METHOD_ARTIST);
      }
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavArtistsViewMethod);

      SetSortOrder(g_stSettings.m_MyMusicNavArtistsSortOrder);
    }
    break;
  case NODE_TYPE_ALBUM:
    {
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(g_stSettings.m_MyMusicNavAlbumsSortMethod);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
      SetViewAsControl(g_stSettings.m_MyMusicNavAlbumsViewMethod);

      SetSortOrder(g_stSettings.m_MyMusicNavAlbumsSortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
      SetViewAsControl(g_stSettings.m_MyMusicNavAlbumsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavSongsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
      SetViewAsControl(g_stSettings.m_MyMusicNavAlbumsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavSongsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
      SetViewAsControl(g_stSettings.m_MyMusicNavAlbumsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_SONG:
    {
      AddSortMethod(SORT_METHOD_TRACKNUM, 266, LABEL_MASKS("%N. %A - %T", "%D"));  // TrackNum, Artist, Title, Duration| empty, empty
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_TITLE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
      AddSortMethod(SORT_METHOD_DURATION, 267, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
      SetSortMethod(g_stSettings.m_MyMusicNavSongsSortMethod);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavSongsViewMethod);

      SetSortOrder(g_stSettings.m_MyMusicNavSongsSortOrder);
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 266, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_METHOD_LIST, 101);
      AddViewAsControl(VIEW_METHOD_ICONS, 100);
      AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
      SetViewAsControl(g_stSettings.m_MyMusicNavSongsViewMethod);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  }
}

void CGUIViewStateMusicDatabase::SaveViewState()
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(m_items.m_strPath);

  switch (NodeType)
  {
  case NODE_TYPE_ROOT:
    {
      g_stSettings.m_MyMusicNavRootViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_OVERVIEW:
    {
      g_stSettings.m_MyMusicNavRootViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_TOP100:
    {
      g_stSettings.m_MyMusicNavTopViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_GENRE:
    {
      g_stSettings.m_MyMusicNavGenresViewMethod=GetViewAsControl();

      g_stSettings.m_MyMusicNavGenresSortOrder=GetSortOrder();
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      g_stSettings.m_MyMusicNavArtistsViewMethod=GetViewAsControl();

      g_stSettings.m_MyMusicNavArtistsSortOrder=GetSortOrder();
    }
    break;
  case NODE_TYPE_ALBUM:
    {
      g_stSettings.m_MyMusicNavAlbumsSortMethod=GetSortMethod();
      
      g_stSettings.m_MyMusicNavAlbumsViewMethod=GetViewAsControl();

      g_stSettings.m_MyMusicNavAlbumsSortOrder=GetSortOrder();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      g_stSettings.m_MyMusicNavAlbumsViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      g_stSettings.m_MyMusicNavSongsViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {      
      g_stSettings.m_MyMusicNavAlbumsViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      g_stSettings.m_MyMusicNavSongsViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      g_stSettings.m_MyMusicNavAlbumsViewMethod=GetViewAsControl();
    }
    break;
  case NODE_TYPE_SONG:
    {
      g_stSettings.m_MyMusicNavSongsSortMethod=GetSortMethod();
      
      g_stSettings.m_MyMusicNavSongsViewMethod=GetViewAsControl();

      g_stSettings.m_MyMusicNavSongsSortOrder=GetSortOrder();
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      g_stSettings.m_MyMusicNavSongsViewMethod=GetViewAsControl();
    }
    break;
  }

  g_settings.Save();
}

CGUIViewStateWindowMusicNav::CGUIViewStateWindowMusicNav(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyMusicNavRootViewMethod);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%D", "%F", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);

    if (items.IsPlayList())
      SetViewAsControl(g_stSettings.m_MyMusicNavPlaylistsViewMethod);
    else  // Songs
      SetViewAsControl(g_stSettings.m_MyMusicNavSongsViewMethod);

    SetSortOrder(SORT_ORDER_ASC);
  }
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
    g_stSettings.m_MyMusicNavRootViewMethod=GetViewAsControl();
  else if (m_items.IsPlayList())
    g_stSettings.m_MyMusicNavPlaylistsViewMethod=GetViewAsControl();
  else // songs
    g_stSettings.m_MyMusicNavSongsViewMethod=GetViewAsControl();

  g_settings.Save();
}

CGUIViewStateWindowMusicSongs::CGUIViewStateWindowMusicSongs(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498, LABEL_MASKS()); // Preformated
    SetSortMethod(g_stSettings.m_MyMusicSongsRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyMusicSongsRootViewMethod);

    SetSortOrder(g_stSettings.m_MyMusicSongsRootSortOrder);
  }
  else
  {
    CStdString strTrackLeft=g_guiSettings.GetString("MyMusic.TrackFormat");
    CStdString strTrackRight=g_guiSettings.GetString("MyMusic.TrackFormatRight");

    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS(strTrackLeft, strTrackRight, "%F", ""));  // Userdefined, Userdefined | FolderName, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS(strTrackLeft, "%I", "%F", "%I"));  // Userdefined, Size | FolderName, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS(strTrackLeft, "%J", "%F", "%J"));  // Userdefined, Date | FolderName, Date
    AddSortMethod(SORT_METHOD_FILE, 363, LABEL_MASKS(strTrackLeft, strTrackRight, "%F", ""));  // Userdefined, Userdefined | FolderName, empty
    SetSortMethod(g_stSettings.m_MyMusicSongsSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("MusicFiles.UseAutoSwitching"))
    {
      SetViewAsControl(CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl(g_stSettings.m_MyMusicSongsViewMethod);
    }

    SetSortOrder(g_stSettings.m_MyMusicSongsSortOrder);
  }
}

void CGUIViewStateWindowMusicSongs::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyMusicSongsRootSortMethod=GetSortMethod();
    g_stSettings.m_MyMusicSongsRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyMusicSongsRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyMusicSongsSortMethod=GetSortMethod();
    g_stSettings.m_MyMusicSongsViewMethod=GetViewAsControl();
    g_stSettings.m_MyMusicSongsSortOrder=GetSortOrder();
  }

  g_settings.Save();
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("MyMusic.TrackFormat");
  CStdString strTrackRight=g_guiSettings.GetString("MyMusic.TrackFormatRight");

  AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight, "%F", ""));  // Userdefined, Userdefined | FolderName, empty
  SetSortMethod(SORT_METHOD_NONE);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_MyMusicPlaylistViewMethod);

  SetSortOrder(SORT_ORDER_NONE);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
  g_stSettings.m_MyMusicPlaylistViewMethod=GetViewAsControl();

  g_settings.Save();
}

int CGUIViewStateWindowMusicPlaylist::GetPlaylist()
{
  return PLAYLIST_MUSIC;
}

bool CGUIViewStateWindowMusicPlaylist::AutoPlayNextItem()
{
  return false;
}
