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

      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavRootViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);

      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavTopViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_GENRE:
    {
      AddSortMethod(SORT_METHOD_LABEL, 103);
      SetSortMethod(SORT_METHOD_LABEL);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavGenresViewAsIcons);

      SetSortOrder(g_stSettings.m_iMyMusicNavGenresSortAscending);
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
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavArtistsViewAsIcons);

      SetSortOrder(g_stSettings.m_iMyMusicNavArtistsSortAscending);
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
      SetSortMethod(g_stSettings.m_iMyMusicNavAlbumsSortMethod);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_LIST, 759);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavAlbumsViewAsIcons);

      SetSortOrder(g_stSettings.m_iMyMusicNavAlbumsSortAscending);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_LIST, 759);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavAlbumsViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavSongsViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_LIST, 759);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavAlbumsViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavSongsViewAsIcons);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_LIST, 759);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavAlbumsViewAsIcons);

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
      SetSortMethod(g_stSettings.m_iMyMusicNavSongsSortMethod);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavSongsViewAsIcons);

      SetSortOrder(g_stSettings.m_iMyMusicNavSongsSortAscending);
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 266);
      SetSortMethod(SORT_METHOD_NONE);
      
      AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
      AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
      AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavSongsViewAsIcons);

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
      g_stSettings.m_iMyMusicNavRootViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_OVERVIEW:
    {
      g_stSettings.m_iMyMusicNavRootViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_TOP100:
    {
      g_stSettings.m_iMyMusicNavTopViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_GENRE:
    {
      g_stSettings.m_iMyMusicNavGenresViewAsIcons=GetViewAsControl();

      g_stSettings.m_iMyMusicNavGenresSortAscending=GetSortOrder();
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      g_stSettings.m_iMyMusicNavArtistsViewAsIcons=GetViewAsControl();

      g_stSettings.m_iMyMusicNavArtistsSortAscending=GetSortOrder();
    }
    break;
  case NODE_TYPE_ALBUM:
    {
      g_stSettings.m_iMyMusicNavAlbumsSortMethod=GetSortMethod();
      
      g_stSettings.m_iMyMusicNavAlbumsViewAsIcons=GetViewAsControl();

      g_stSettings.m_iMyMusicNavAlbumsSortAscending=GetSortOrder();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      g_stSettings.m_iMyMusicNavAlbumsViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      g_stSettings.m_iMyMusicNavSongsViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {      
      g_stSettings.m_iMyMusicNavAlbumsViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      g_stSettings.m_iMyMusicNavSongsViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      g_stSettings.m_iMyMusicNavAlbumsViewAsIcons=GetViewAsControl();
    }
    break;
  case NODE_TYPE_SONG:
    {
      g_stSettings.m_iMyMusicNavSongsSortMethod=GetSortMethod();
      
      g_stSettings.m_iMyMusicNavSongsViewAsIcons=GetViewAsControl();

      g_stSettings.m_iMyMusicNavSongsSortAscending=GetSortOrder();
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      g_stSettings.m_iMyMusicNavSongsViewAsIcons=GetViewAsControl();
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

    AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
    AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
    AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
    SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavRootViewAsIcons);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    SetSortMethod(SORT_METHOD_LABEL);

    AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
    AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
    AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);

    if (items.IsPlayList())
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavPlaylistsViewAsIcons);
    else  // Songs
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicNavSongsViewAsIcons);

    SetSortOrder(SORT_ORDER_ASC);
  }
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
    g_stSettings.m_iMyMusicNavRootViewAsIcons=GetViewAsControl();
  else if (m_items.IsPlayList())
    g_stSettings.m_iMyMusicNavPlaylistsViewAsIcons=GetViewAsControl();
  else // songs
    g_stSettings.m_iMyMusicNavSongsViewAsIcons=GetViewAsControl();

  g_settings.Save();
}

CGUIViewStateWindowMusicSongs::CGUIViewStateWindowMusicSongs(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498);
    SetSortMethod((SORT_METHOD)g_stSettings.m_iMyMusicSongsRootSortMethod);

    AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
    AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
    AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
    SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicSongsRootViewAsIcons);

    SetSortOrder(g_stSettings.m_iMyMusicSongsRootSortAscending);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_SIZE, 105);
    AddSortMethod(SORT_METHOD_DATE, 104);
    AddSortMethod(SORT_METHOD_FILE, 363);
    SetSortMethod((SORT_METHOD)g_stSettings.m_iMyMusicSongsSortMethod);

    AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
    AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
    AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("MusicFiles.UseAutoSwitching"))
    {
      SetViewAsControl((VIEW_AS_CONTROL)CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicSongsViewAsIcons);
    }

    SetSortOrder(g_stSettings.m_iMyMusicSongsSortAscending);
  }
}

void CGUIViewStateWindowMusicSongs::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_iMyMusicSongsRootSortMethod=GetSortMethod();
    g_stSettings.m_iMyMusicSongsRootViewAsIcons=GetViewAsControl();
    g_stSettings.m_iMyMusicSongsRootSortAscending=GetSortOrder();
  }
  else
  {
    g_stSettings.m_iMyMusicSongsSortMethod=GetSortMethod();
    g_stSettings.m_iMyMusicSongsViewAsIcons=GetViewAsControl();
    g_stSettings.m_iMyMusicSongsSortAscending=GetSortOrder();
  }

  g_settings.Save();
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_NONE, 103);
  SetSortMethod(SORT_METHOD_NONE);

  AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
  AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
  AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
  SetViewAsControl((VIEW_AS_CONTROL)g_stSettings.m_iMyMusicPlaylistViewAsIcons);

  SetSortOrder(SORT_ORDER_NONE);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
  g_stSettings.m_iMyMusicPlaylistViewAsIcons=GetViewAsControl();

  g_settings.Save();
}
