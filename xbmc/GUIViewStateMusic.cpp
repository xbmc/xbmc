#include "stdafx.h"
#include "GUIViewStateMusic.h"
#include "AutoSwitch.h"
#include "playlistplayer.h"
#include "util.h"
#include "GUIBaseContainer.h" // for VIEW_TYPE_*

#include "filesystem/musicdatabasedirectory.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

int CGUIViewStateWindowMusic::GetPlaylist()
{
  //return PLAYLIST_MUSIC_TEMP;
  return PLAYLIST_MUSIC;
}

bool CGUIViewStateWindowMusic::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

bool CGUIViewStateWindowMusic::AutoPlayNextItem()
{
  return g_guiSettings.GetBool("mymusic.autoplaynextitem");
}

CStdString CGUIViewStateWindowMusic::GetLockType()
{
  return "music";
}

CStdString CGUIViewStateWindowMusic::GetExtensions()
{
  return g_stSettings.m_musicExtensions;
}

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(items.m_strPath);
  NODE_TYPE ParentNodeType=dir.GetDirectoryType(items.m_strPath);

  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.librarytrackformat");
  if (strTrackLeft.IsEmpty())
    strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.librarytrackformatright");
  if (strTrackRight.IsEmpty())
    strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SORT_METHOD_NONE);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SORT_METHOD_NONE);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_GENRE:
    {
      AddSortMethod(SORT_METHOD_GENRE, 103, LABEL_MASKS("%F", "", "%G", ""));  // Filename, empty | Genre, empty
      SetSortMethod(SORT_METHOD_GENRE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_ASC);
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      {
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 103, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
        SetSortMethod(SORT_METHOD_ARTIST_IGNORE_THE);
      }
      else
      {
        AddSortMethod(SORT_METHOD_ARTIST, 103, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
        SetSortMethod(SORT_METHOD_ARTIST);
      }
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_ASC);
    }
    break;
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM:
    {
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_ALBUM);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_ASC);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%B", "%A"));  // Filename, empty | Album, Artist
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
  case NODE_TYPE_SONG:
    {
      AddSortMethod(SORT_METHOD_TRACKNUM, 266, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_TITLE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
      else
        AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
      AddSortMethod(SORT_METHOD_DURATION, 267, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
      SetSortMethod(SORT_METHOD_TRACKNUM);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_ASC);
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      AddSortMethod(SORT_METHOD_NONE, 266, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SORT_METHOD_NONE);
      
      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SORT_ORDER_NONE);
    }
    break;
  }

  LoadViewState(items.m_strPath, WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicDatabase::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_NAV);
}


CGUIViewStateMusicSmartPlaylist::CGUIViewStateMusicSmartPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");
  // TODO: localize 2.0
  AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 20014, LABEL_MASKS(strTrackLeft, strTrackRight));
  AddSortMethod(SORT_METHOD_TRACKNUM, 266, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_TITLE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
  AddSortMethod(SORT_METHOD_DURATION, 267, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
  SetSortMethod(SORT_METHOD_TRACKNUM);
      
  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSmartPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_NAV);
}

CGUIViewStateMusicPlaylist::CGUIViewStateMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");
  // TODO: localize 2.0
  AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 20014, LABEL_MASKS(strTrackLeft, strTrackRight));
  AddSortMethod(SORT_METHOD_TRACKNUM, 266, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_TITLE, 268, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_ALBUM, 270, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
  else
    AddSortMethod(SORT_METHOD_ARTIST, 269, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
  AddSortMethod(SORT_METHOD_DURATION, 267, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
  SetSortMethod(SORT_METHOD_LABEL);
      
  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_NAV);
}


CGUIViewStateWindowMusicNav::CGUIViewStateWindowMusicNav(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_NONE);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  LoadViewState(items.m_strPath, WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_NAV);
}

VECSHARES& CGUIViewStateWindowMusicNav::GetShares()
{
  //  Setup shares we want to have
  m_shares.clear();
  //  Musicdb shares
  CFileItemList items;
  CDirectory::GetDirectory("musicdb://", items);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItem* item=items[i];
    CShare share;
    share.strName=item->GetLabel();
    share.strPath = item->m_strPath;
    share.m_strThumbnailImage="defaultFolderBig.png";
    share.m_iDriveType = SHARE_TYPE_LOCAL;
    m_shares.push_back(share);
  }

  //  Playlists share
  CShare share;
  share.strName=g_localizeStrings.Get(136); // Playlists
  share.strPath = "special://musicplaylists/";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowMusic::GetShares();
}

CGUIViewStateWindowMusicSongs::CGUIViewStateWindowMusicSongs(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498, LABEL_MASKS()); // Preformated
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_ICONS);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
    CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS(strTrackLeft, "%I", "%L", "%I"));  // Userdefined, Size | FolderName, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS(strTrackLeft, "%J", "%L", "%J"));  // Userdefined, Date | FolderName, Date
    AddSortMethod(SORT_METHOD_FILE, 363, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
    SetSortMethod(SORT_METHOD_LABEL);

    if (g_guiSettings.GetBool("musicfiles.useautoswitching"))
    {
      SetViewAsControl(CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl(DEFAULT_VIEW_LIST);
    }

    SetSortOrder(SORT_ORDER_ASC);
  }
  LoadViewState(items.m_strPath, WINDOW_MUSIC_FILES);
}

void CGUIViewStateWindowMusicSongs::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_FILES);
}

VECSHARES& CGUIViewStateWindowMusicSongs::GetShares()
{
  return g_settings.m_vecMyMusicShares;
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.nowplayingtrackformat");
  if (strTrackLeft.IsEmpty())
    strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.nowplayingtrackformatright");
  if (strTrackRight.IsEmpty())
    strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");

  AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
  SetSortMethod(SORT_METHOD_NONE);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_NONE);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
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

bool CGUIViewStateWindowMusicPlaylist::HideParentDirItems()
{
  return true;
}

VECSHARES& CGUIViewStateWindowMusicPlaylist::GetShares()
{
  m_shares.clear();
  //  Playlist share
  CShare share;
  share.strName;
  share.strPath = "playlistmusic://";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowMusic::GetShares();
}

CGUIViewStateMusicShoutcast::CGUIViewStateMusicShoutcast(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  /* sadly m_idepth isn't remembered when a directory is retrieved from cache */
  /* and thus this check hardly ever works, so let's just disable it for now */
  if( true || m_items.m_idepth > 1 )
  { /* station list */
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%B kbps", "%K", ""));  // Title, Bitrate | Title, nothing
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 507, LABEL_MASKS("%K", "%A listeners", "%K", ""));  // Titel, Listeners | Titel, nothing
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%K", "%B kbps", "%K", ""));  // Title, Bitrate | Title, nothing

    SetSortMethod(SORT_METHOD_VIDEO_RATING);
    SetSortOrder(SORT_ORDER_DESC);
  }
  else
  { /* genre list */
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "", "%K", ""));  // Title, nothing | Title, nothing
    SetSortMethod(SORT_METHOD_LABEL);
    SetSortOrder(SORT_ORDER_ASC); /* maybe we should have this stored somewhere */
  }

  SetViewAsControl(DEFAULT_VIEW_LIST);
  LoadViewState(items.m_strPath, WINDOW_MUSIC_FILES);
}

bool CGUIViewStateMusicShoutcast::AutoPlayNextItem()
{
  return false;
}

void CGUIViewStateMusicShoutcast::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_FILES);
}

CGUIViewStateMusicLastFM::CGUIViewStateMusicLastFM(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

  AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
  AddSortMethod(SORT_METHOD_SIZE, 507, LABEL_MASKS(strTrackLeft, "%I", "%L", "%I"));  // Userdefined, Size | FolderName, Size

  SetSortMethod(SORT_METHOD_LABEL);
  SetSortOrder(SORT_ORDER_ASC);
  
  SetViewAsControl(DEFAULT_VIEW_LIST);
  LoadViewState(items.m_strPath, WINDOW_MUSIC_FILES);
}

bool CGUIViewStateMusicLastFM::AutoPlayNextItem()
{
  return false;
}

void CGUIViewStateMusicLastFM::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_MUSIC_FILES);
}