#include "stdafx.h"
#include "GUIViewStateVideo.h"
#include "AutoSwitch.h"
#include "playlistplayer.h"
#include "FileSystem/VideoDatabaseDirectory.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CStdString CGUIViewStateWindowVideo::GetLockType()
{
  return "video";
}

bool CGUIViewStateWindowVideo::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowVideo::GetExtensions()
{
  return g_stSettings.m_videoExtensions;
}

CGUIViewStateWindowVideoFiles::CGUIViewStateWindowVideoFiles(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498, LABEL_MASKS()); // Preformated
    SetSortMethod(g_stSettings.m_MyVideoRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoRootSortOrder);
  }
  else
  {
    // always use the label by default
    CStdString strFileMask = "%L";
    // when stacking is enabled, filenames are already cleaned so use the existing label
    if (g_stSettings.m_iMyVideoStack != STACK_NONE)
      strFileMask = "%L"; 
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS(strFileMask, "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS(strFileMask, "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    SetSortMethod(g_stSettings.m_MyVideoSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("myvideos.useautoswitching"))
    {
      SetViewAsControl(CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl(g_stSettings.m_MyVideoViewMethod);
    }

    SetSortOrder(g_stSettings.m_MyVideoSortOrder);
  }
}

void CGUIViewStateWindowVideoFiles::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

VECSHARES& CGUIViewStateWindowVideoFiles::GetShares()
{
  return g_settings.m_vecMyVideoShares;
}

CGUIViewStateWindowVideoNav::CGUIViewStateWindowVideoNav(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoNavRootViewMethod);

    SetSortOrder(SORT_ORDER_NONE);
  }
  else if (items.IsVideoDb())
  {
    CVideoDatabaseDirectory dir;
    NODE_TYPE NodeType=dir.GetDirectoryChildType(items.m_strPath);
    NODE_TYPE ParentNodeType=dir.GetDirectoryType(items.m_strPath);
    switch (NodeType)
    {
    case NODE_TYPE_OVERVIEW:
      {
        AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_NONE);

        AddViewAsControl(VIEW_METHOD_LIST, 101);
        AddViewAsControl(VIEW_METHOD_ICONS, 100);
        AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
        SetViewAsControl(g_stSettings.m_MyMusicNavRootViewMethod);

        SetSortOrder(SORT_ORDER_NONE);
      }
      break;
    case NODE_TYPE_ACTOR:
      {
        AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_LABEL);

        AddViewAsControl(VIEW_METHOD_LIST, 101);
        AddViewAsControl(VIEW_METHOD_ICONS, 100);
        AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
        SetViewAsControl(g_stSettings.m_MyVideoNavActorViewMethod);

        SetSortOrder(g_stSettings.m_MyVideoNavActorSortOrder);
      }
      break;
    case NODE_TYPE_YEAR:
            {
        AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_LABEL);

        AddViewAsControl(VIEW_METHOD_LIST, 101);
        AddViewAsControl(VIEW_METHOD_ICONS, 100);
        AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
        SetViewAsControl(g_stSettings.m_MyVideoNavYearViewMethod);

        SetSortOrder(g_stSettings.m_MyVideoNavYearSortOrder);
      }
      break;
    case NODE_TYPE_GENRE:
      {
        AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(g_stSettings.m_MyVideoNavGenreSortMethod);

        AddViewAsControl(VIEW_METHOD_LIST, 101);
        AddViewAsControl(VIEW_METHOD_ICONS, 100);
        AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
        SetViewAsControl(g_stSettings.m_MyVideoNavGenreViewMethod);

        SetSortOrder(g_stSettings.m_MyVideoNavGenreSortOrder);
      }
      break;
      case NODE_TYPE_TITLE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        else
          AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(g_stSettings.m_MyVideoNavTitleSortMethod);

        AddViewAsControl(VIEW_METHOD_LIST, 101);
        AddViewAsControl(VIEW_METHOD_ICONS, 100);
        AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
        AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
        SetViewAsControl(g_stSettings.m_MyVideoNavTitleViewMethod);

        SetSortOrder(g_stSettings.m_MyVideoNavTitleSortOrder);
      }
      break;
    }
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoNavTitleSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);

    SetViewAsControl(g_stSettings.m_MyVideoNavPlaylistsViewMethod);

  }
}

void CGUIViewStateWindowVideoNav::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoNavRootViewMethod=GetViewAsControl();
  }
  else if (m_items.IsPlayList())
  {
    g_stSettings.m_MyVideoNavPlaylistsViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoNavPlaylistsSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoNavPlaylistsSortOrder=GetSortOrder();
  }
  else
  {
    CVideoDatabaseDirectory dir;
    NODE_TYPE type = dir.GetDirectoryChildType(m_items.m_strPath);
    if (type == NODE_TYPE_GENRE)
    {
      g_stSettings.m_MyVideoNavGenreViewMethod=GetViewAsControl();
      g_stSettings.m_MyVideoNavGenreSortMethod=GetSortMethod();
      g_stSettings.m_MyVideoNavGenreSortOrder=GetSortOrder();
    }
    if (type == NODE_TYPE_TITLE)
    {
      g_stSettings.m_MyVideoNavTitleViewMethod=GetViewAsControl();
      g_stSettings.m_MyVideoNavTitleSortMethod=GetSortMethod();
      g_stSettings.m_MyVideoNavTitleSortOrder=GetSortOrder();      
    }
    if (type == NODE_TYPE_ACTOR)
    {
      g_stSettings.m_MyVideoNavActorViewMethod=GetViewAsControl();
      g_stSettings.m_MyVideoNavActorSortOrder=GetSortOrder();      
    }
    if (type == NODE_TYPE_YEAR)
    {
      g_stSettings.m_MyVideoNavYearViewMethod=GetViewAsControl();
      g_stSettings.m_MyVideoNavYearSortOrder=GetSortOrder();      
    }
  }

  g_settings.Save();
}

VECSHARES& CGUIViewStateWindowVideoNav::GetShares()
{
  //  Setup shares we want to have
  m_shares.clear();
  //  Musicdb shares
  CFileItemList items;
  CDirectory::GetDirectory("videodb://", items);
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
  share.strPath = "special://videoplaylists/";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowVideo::GetShares();
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%L", "", "%L", ""));  // Label, "" | Label, empty
  SetSortMethod(SORT_METHOD_NONE);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_MyVideoPlaylistViewMethod);

  SetSortOrder(SORT_ORDER_NONE);
}

void CGUIViewStateWindowVideoPlaylist::SaveViewState()
{
  g_stSettings.m_MyVideoPlaylistViewMethod=GetViewAsControl();
  g_settings.Save();
}

int CGUIViewStateWindowVideoPlaylist::GetPlaylist()
{
  return PLAYLIST_VIDEO;
}

bool CGUIViewStateWindowVideoPlaylist::HideExtensions()
{
  return true;
}

bool CGUIViewStateWindowVideoPlaylist::HideParentDirItems()
{
  return true;
}

VECSHARES& CGUIViewStateWindowVideoPlaylist::GetShares()
{
  m_shares.clear();
  //  Playlist share
  CShare share;
  share.strName;
  share.strPath= "playlistvideo://";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowVideo::GetShares();
}
