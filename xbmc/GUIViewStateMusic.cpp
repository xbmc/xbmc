#include "stdafx.h"
#include "GUIViewStateMusic.h"
#include "AutoSwitch.h"

#include "filesystem/musicdatabasedirectory.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewState(items)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(items.m_strPath);

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_GENRE, 103);
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
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 103);
        SetSortMethod(SORT_METHOD_ARTIST_IGNORE_THE);
      }
      else
      {
        AddSortMethod(SORT_METHOD_ARTIST, 103);
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
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270);
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270);
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269);
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_NONE, 103);
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
      AddSortMethod(SORT_METHOD_TRACKNUM, 266);
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270);
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270);
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269);
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269);
      if (g_guiSettings.GetBool("MyMusic.IgnoreTheWhenSorting"))
        AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 268);
      else
        AddSortMethod(SORT_METHOD_TITLE, 268);
      AddSortMethod(SORT_METHOD_DURATION, 267);
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
      AddSortMethod(SORT_METHOD_NONE, 266);
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

CGUIViewStateWindowMusicNav::CGUIViewStateWindowMusicNav(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 103);
    SetSortMethod(SORT_METHOD_NONE);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyMusicNavRootViewMethod);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
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

CGUIViewStateWindowMusicSongs::CGUIViewStateWindowMusicSongs(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498);
    SetSortMethod(g_stSettings.m_MyMusicSongsRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyMusicSongsRootViewMethod);

    SetSortOrder(g_stSettings.m_MyMusicSongsRootSortOrder);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_SIZE, 105);
    AddSortMethod(SORT_METHOD_DATE, 104);
    AddSortMethod(SORT_METHOD_FILE, 363);
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

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_NONE, 103);
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
