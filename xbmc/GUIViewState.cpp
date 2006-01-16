#include "stdafx.h"
#include "GUIViewState.h"
#include "GUIViewStateMusic.h"
#include "GUIViewStateVideo.h"
#include "GUIViewStatePicturesProgramsScripts.h"
#include "playlistplayer.h"

CStdString CGUIViewState::m_strPlaylistDirectory;

CGUIViewState* CGUIViewState::GetViewState(int windowId, const CFileItemList& items)
{
  const CURL& url=items.GetAsUrl();

  if (url.GetProtocol()=="musicdb")
    return new CGUIViewStateMusicDatabase(items);

  if (windowId==WINDOW_MUSIC_NAV)
    return new CGUIViewStateWindowMusicNav(items);

  if (windowId==WINDOW_MUSIC_FILES)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST)
    return new CGUIViewStateWindowMusicPlaylist(items);

  if (windowId==WINDOW_VIDEOS)
    return new CGUIViewStateWindowVideoFiles(items);

  if (windowId==WINDOW_VIDEO_GENRE)
    return new CGUIViewStateWindowVideoGenre(items);

  if (windowId==WINDOW_VIDEO_ACTOR)
    return new CGUIViewStateWindowVideoActor(items);

  if (windowId==WINDOW_VIDEO_YEAR)
    return new CGUIViewStateWindowVideoYear(items);

  if (windowId==WINDOW_VIDEO_TITLE)
    return new CGUIViewStateWindowVideoTitle(items);

  if (windowId==WINDOW_VIDEO_PLAYLIST)
    return new CGUIViewStateWindowVideoPlaylist(items);

  if (windowId==WINDOW_SCRIPTS)
    return new CGUIViewStateWindowScripts(items);

  if (windowId==WINDOW_PICTURES)
    return new CGUIViewStateWindowPictures(items);

  if (windowId==WINDOW_PROGRAMS)
    return new CGUIViewStateWindowPrograms(items);

  //  Use as fallback/default
  return new CGUIViewStateGeneral(items);
}

CGUIViewState::CGUIViewState(const CFileItemList& items) : m_items(items)
{
  m_currentViewAsControl=0;
  m_currentSortMethod=0;
  m_sortOrder=SORT_ORDER_ASC;
  m_hideParentDirItems = items.IsVirtualDirectoryRoot() ? true : g_guiSettings.GetBool("FileLists.HideParentDirItems");
}

CGUIViewState::~CGUIViewState()
{
}

SORT_ORDER CGUIViewState::SetNextSortOrder()
{
  if (m_sortOrder==SORT_ORDER_ASC)
    m_sortOrder=SORT_ORDER_DESC;
  else
    m_sortOrder=SORT_ORDER_ASC;

  SaveViewState();

  return m_sortOrder;
}

VIEW_METHOD CGUIViewState::GetViewAsControl() const
{
  if (m_currentViewAsControl>=0 && m_currentViewAsControl<(int)m_viewAsControls.size())
    return m_viewAsControls[m_currentViewAsControl].m_viewAsControl;

  return VIEW_METHOD_LIST;
}

int CGUIViewState::GetViewAsControlButtonLabel() const
{
  if (m_currentViewAsControl>=0 && m_currentViewAsControl<(int)m_viewAsControls.size())
    return m_viewAsControls[m_currentViewAsControl].m_buttonLabel;

  return 101; // "View As: List" button label
}

void CGUIViewState::AddViewAsControl(VIEW_METHOD viewAsControl, int buttonLabel)
{
  VIEW view;
  view.m_viewAsControl=viewAsControl;
  view.m_buttonLabel=buttonLabel;

  m_viewAsControls.push_back(view);
}

void CGUIViewState::SetViewAsControl(VIEW_METHOD viewAsControl)
{
  for (int i=0; i<(int)m_viewAsControls.size(); ++i)
  {
    if (m_viewAsControls[i].m_viewAsControl==viewAsControl)
    {
      m_currentViewAsControl=i;
      break;
    }
  }
}

VIEW_METHOD CGUIViewState::SetNextViewAsControl()
{
  m_currentViewAsControl++;

  if (m_currentViewAsControl>=(int)m_viewAsControls.size())
    m_currentViewAsControl=0;

  SaveViewState();

  return GetViewAsControl();
}

SORT_METHOD CGUIViewState::GetSortMethod() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_sortMethod;

  return SORT_METHOD_NONE;
}

int CGUIViewState::GetSortMethodLabel() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_buttonLabel; 

  return 103; // Sort By: Name
}

void CGUIViewState::GetSortMethodLabelMasks(LABEL_MASKS& masks) const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
  {
    masks=m_sortMethods[m_currentSortMethod].m_labelMasks; 
    return;
  }

  masks.m_strLabelFile.Empty();
  masks.m_strLabel2File.Empty();
  masks.m_strLabelFolder.Empty();
  masks.m_strLabel2Folder.Empty();
  return;
}

void CGUIViewState::AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks)
{
  SORT sort;
  sort.m_sortMethod=sortMethod;
  sort.m_buttonLabel=buttonLabel;
  sort.m_labelMasks=labelmasks;

  m_sortMethods.push_back(sort);
}

void CGUIViewState::SetSortMethod(SORT_METHOD sortMethod)
{
  for (int i=0; i<(int)m_sortMethods.size(); ++i)
  {
    if (m_sortMethods[i].m_sortMethod==sortMethod)
    {
      m_currentSortMethod=i;
      break;
    }
  }
}

SORT_METHOD CGUIViewState::SetNextSortMethod()
{
  m_currentSortMethod++;

  if (m_currentSortMethod>=(int)m_sortMethods.size())
    m_currentSortMethod=0;

  SaveViewState();

  return GetSortMethod();
}

bool CGUIViewState::HideExtensions()
{
  return g_guiSettings.GetBool("FileLists.HideExtensions");
}

bool CGUIViewState::HideParentDirItems()
{
  return m_hideParentDirItems;
}

int CGUIViewState::GetPlaylist()
{
  return PLAYLIST_NONE;
}

const CStdString& CGUIViewState::GetPlaylistDirectory()
{
  return m_strPlaylistDirectory;
}

void CGUIViewState::SetPlaylistDirectory(const CStdString& strDirectory)
{
  m_strPlaylistDirectory=strDirectory;
}

bool CGUIViewState::HandleArchives()
{
  return false;
}

bool CGUIViewState::AutoPlayNextItem()
{
  return false;
}

CStdString CGUIViewState::GetLockType()
{
  return "";
}

CGUIViewStateGeneral::CGUIViewStateGeneral(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // Filename, size | Foldername, empty
  SetSortMethod(SORT_METHOD_LABEL);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(VIEW_METHOD_LIST);

  SetSortOrder(SORT_ORDER_ASC);
}
