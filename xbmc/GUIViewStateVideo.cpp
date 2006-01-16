#include "stdafx.h"
#include "GUIViewStateVideo.h"
#include "AutoSwitch.h"
#include "playlistplayer.h"

CGUIViewStateWindowVideo::CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items)
{
  if (!items.IsVirtualDirectoryRoot())
    m_hideParentDirItems = g_guiSettings.GetBool("MyVideos.HideParentDirItems");
}

CStdString CGUIViewStateWindowVideo::GetLockType()
{
  return "video";
}

bool CGUIViewStateWindowVideo::HandleArchives()
{
  return g_guiSettings.GetBool("VideoFiles.HandleArchives");
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
    if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // FileName, Size | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%F", "%I", "%F", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%F", "%J", "%F", "%J"));  // FileName, Date | Foldername, Date
    SetSortMethod(g_stSettings.m_MyVideoSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("VideoFiles.UseAutoSwitching"))
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

CGUIViewStateWindowVideoTitle::CGUIViewStateWindowVideoTitle(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
  AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Q", "%F", ""));  // Titel, Year | Foldername, empty
  AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
  SetSortMethod(g_stSettings.m_MyVideoTitleSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
  SetViewAsControl(g_stSettings.m_MyVideoTitleViewMethod);

  SetSortOrder((SORT_ORDER)g_stSettings.m_MyVideoTitleSortOrder);
}

void CGUIViewStateWindowVideoTitle::SaveViewState()
{
  g_stSettings.m_MyVideoTitleSortMethod=GetSortMethod();
  g_stSettings.m_MyVideoTitleViewMethod=GetViewAsControl();
  g_stSettings.m_MyVideoTitleSortOrder=GetSortOrder();
  g_settings.Save();
}

CGUIViewStateWindowVideoGenre::CGUIViewStateWindowVideoGenre(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%F", ""));  // Filename, Empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoGenreRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoGenreRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoGenreRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Q", "%F", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoGenreSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoGenreViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoGenreSortOrder);
  }
}

void CGUIViewStateWindowVideoGenre::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoGenreRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoGenreRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoGenreRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoGenreSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoGenreViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoGenreSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoActor::CGUIViewStateWindowVideoActor(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%F", ""));  // FileName, empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoActorRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoActorRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoActorRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Q", "%F", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoActorSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoActorViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoActorSortOrder);
  }
}

void CGUIViewStateWindowVideoActor::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoActorRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoActorRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoActorRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoActorSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoActorViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoActorSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoYear::CGUIViewStateWindowVideoYear(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%F", ""));  // Filename, empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoYearRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoYearRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoYearRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Q", "%F", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%F", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoYearSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoYearViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoYearSortOrder);
  }
}

void CGUIViewStateWindowVideoYear::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoYearRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoYearRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoYearRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoYearSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoYearViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoYearSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%F", "", "%F", ""));  // FileName, "" | Foldername, empty
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

