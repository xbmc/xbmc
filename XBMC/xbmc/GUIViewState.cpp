#include "stdafx.h"
#include "GUIViewState.h"
#include "GUIViewStateMusic.h"


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

  //  Use as fallback/default
  return new CGUIViewStateGeneral(items);
}

CGUIViewState::CGUIViewState(const CFileItemList& items) : m_items(items)
{
  m_currentViewAsControl=0;
  m_currentSortMethod=0;
  m_sortOrder=SORT_ORDER_ASC;
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

VIEW_AS_CONTROL CGUIViewState::GetViewAsControl() const
{
  if (m_currentViewAsControl>=0 && m_currentViewAsControl<(int)m_viewAsControls.size())
    return m_viewAsControls[m_currentViewAsControl].m_viewAsControl;

  return VIEW_AS_CONTROL_LIST;
}

int CGUIViewState::GetViewAsControlButtonLabel() const
{
  if (m_currentViewAsControl>=0 && m_currentViewAsControl<(int)m_viewAsControls.size())
    return m_viewAsControls[m_currentViewAsControl].m_buttonLabel;

  return VIEW_AS_CONTROL_LIST;
}

void CGUIViewState::AddViewAsControl(VIEW_AS_CONTROL viewAsControl, int buttonLabel)
{
  VIEW view;
  view.m_viewAsControl=viewAsControl;
  view.m_buttonLabel=buttonLabel;

  m_viewAsControls.push_back(view);
}

void CGUIViewState::SetViewAsControl(VIEW_AS_CONTROL viewAsControl)
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

VIEW_AS_CONTROL CGUIViewState::SetNextViewAsControl()
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

  return SORT_METHOD_NONE;
}

void CGUIViewState::AddSortMethod(SORT_METHOD sortMethod, int buttonLabel)
{
  SORT sort;
  sort.m_sortMethod=sortMethod;
  sort.m_buttonLabel=buttonLabel;

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

CGUIViewStateGeneral::CGUIViewStateGeneral(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103);
  SetSortMethod(SORT_METHOD_LABEL);

  AddViewAsControl(VIEW_AS_CONTROL_LIST, 101);
  AddViewAsControl(VIEW_AS_CONTROL_ICONS, 100);
  AddViewAsControl(VIEW_AS_CONTROL_LARGE_ICONS, 417);
  SetViewAsControl(VIEW_AS_CONTROL_LIST);

  SetSortOrder(SORT_ORDER_ASC);
}
