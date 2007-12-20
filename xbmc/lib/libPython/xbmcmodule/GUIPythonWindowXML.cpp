#include "stdafx.h"
#include "GUIPythonWindowXML.h"
#include "guiwindow.h"
#include "pyutil.h"
#include "../../../application.h"
#include "../../../GUIMediaWindow.h"
#include "window.h"
#include "control.h"
#include "action.h"
#include "../../../util.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_LIST            50
#define CONTROL_LABELFILES      12

#define CONTROL_VIEW_START      50
#define CONTROL_VIEW_END        59


using namespace PYXBMC;

CGUIPythonWindowXML::CGUIPythonWindowXML(DWORD dwId, CStdString strXML, CStdString strFallBackPath)
: CGUIWindow(dwId, strXML)
{
  pCallbackWindow = NULL;
  m_actionEvent = CreateEvent(NULL, true, false, NULL);
  m_loadOnDemand = false;
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  m_coordsRes = PAL_4x3;
  m_fallbackPath = strFallBackPath;
}

CGUIPythonWindowXML::~CGUIPythonWindowXML(void)
{
    CloseHandle(m_actionEvent);
}
void CGUIPythonWindowXML::Update()
{
}
bool CGUIPythonWindowXML::OnAction(const CAction &action)
{
  // do the base class window first, and the call to python after this
  bool ret = CGUIWindow::OnAction(action);
  if(pCallbackWindow)
  {
    PyXBMCAction* inf = new PyXBMCAction;
    inf->pObject = Action_FromAction(action);
    inf->pCallbackWindow = pCallbackWindow;

    // aquire lock?
    Py_AddPendingCall(Py_XBMC_Event_OnAction, inf);
    PulseActionEvent();
  }
  return ret;
}

void CGUIPythonWindowXML::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  vector<CGUIControl *> controls;
  GetContainers(controls);
  for (ciControls it = controls.begin(); it != controls.end(); it++)
  {
    CGUIControl *control = *it;
    if (control->GetID() >= CONTROL_VIEW_START && control->GetID() <= CONTROL_VIEW_END)
      m_viewControl.AddView(control);
  }
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
  UpdateButtons();
}

bool CGUIPythonWindowXML::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      PyXBMCAction* inf = new PyXBMCAction;
      inf->pObject = NULL;
      // create a new call and set it in the python queue
      inf->pCallbackWindow = pCallbackWindow;
      Py_AddPendingCall(Py_XBMC_Event_OnInit, inf);
      return true;
    }
    break;

    case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
        // check if our focused control is one of our category buttons
        int iControl=message.GetControlId();
        if(pCallbackWindow)
        {
          PyXBMCAction* inf = new PyXBMCAction;
          inf->pObject = NULL;
          // create a new call and set it in the python queue
          inf->pCallbackWindow = pCallbackWindow;
          inf->controlId = iControl;
          // aquire lock?
          Py_AddPendingCall(Py_XBMC_Event_OnFocus, inf);
          PulseActionEvent();
        }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      // Handle Sort/View internally. Scripters shouldn't use ID 2, 3 or 4.

      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if (m_guiState.get())
        {
          m_guiState->SaveViewAsControl(m_viewControl.GetNextViewMode());
        }
        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        CLog::Log(LOGINFO, "WindowXML: Internal asc/dsc button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();*/
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        CLog::Log(LOGINFO, "WindowXML: Internal sort button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();*/
        return true;
      }

      if(pCallbackWindow && iControl && iControl != this->GetID()) // pCallbackWindow &&  != this->GetID())
      {
        CGUIControl* controlClicked = (CGUIControl*)this->GetControl(iControl);

        // The old python way used to check list AND SELECITEM method or if its a button, checkmark.
        // Its done this way for now to allow other controls without a python version like togglebutton to still raise a onAction event
        if (controlClicked) // Will get problems if we the id is not on the window and we try to do GetControlType on it. So check to make sure it exists
        {
          if (controlClicked->GetControlType() == CGUIControl::GUICONTAINER_LIST &&  message.GetParam1() == ACTION_SELECT_ITEM  || controlClicked->GetControlType() != CGUIControl::GUICONTAINER_LIST)
          {
            PyXBMCAction* inf = new PyXBMCAction;
            inf->pObject = NULL;
            // create a new call and set it in the python queue
            inf->pCallbackWindow = pCallbackWindow;
            inf->controlId = iControl;
            // aquire lock?
            Py_AddPendingCall(Py_XBMC_Event_OnClick, inf);
            PulseActionEvent();
          }
        }
      }
    }
    break;

    case GUI_MSG_CHANGE_VIEW_MODE:
    {
      int viewMode = 0;
      if (message.GetParam1())  // we have an id
        viewMode = m_viewControl.GetViewModeByID(message.GetParam1());
      else if (message.GetParam2())
        viewMode = m_viewControl.GetNextViewMode((int)message.GetParam2());

      if (m_guiState.get())
        m_guiState->SaveViewAsControl(viewMode);
      UpdateButtons();
      return true;
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowXML::AddItem(CFileItem * fileItem, int itemPosition)
{
  if (itemPosition == INT_MAX || itemPosition > m_vecItems.Size())
  {
    m_vecItems.Add(fileItem);
  }
  else if (itemPosition <  -1 &&  !(itemPosition*-1 < m_vecItems.Size()))
  {
    m_vecItems.AddFront(fileItem,0);
  } 
  else
  {
    m_vecItems.AddFront(fileItem,itemPosition);
  }
  m_viewControl.SetItems(m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::RemoveItem(int itemPosition)
{
  m_vecItems.Remove(itemPosition);
  m_viewControl.SetItems(m_vecItems);
  UpdateButtons();
}

int CGUIPythonWindowXML::GetListSize()
{
  return m_vecItems.Size();
}

int CGUIPythonWindowXML::GetCurrentListPosition()
{
  return m_viewControl.GetSelectedItem();
}

void CGUIPythonWindowXML::SetCurrentListPosition(int item)
{
  m_viewControl.SetSelectedItem(item);
}

CFileItem * CGUIPythonWindowXML::GetListItem(int position)
{ 
  if (position < 0 || position >= m_vecItems.Size()) return NULL;
  return m_vecItems[position];
}

CFileItem *CGUIPythonWindowXML::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (item < 0 || !m_vecItems.Size()) return NULL;

  item = (item + offset) % m_vecItems.Size();
  if (item < 0) item += m_vecItems.Size();
  return m_vecItems[item];
}


void CGUIPythonWindowXML::ClearList()
{
  m_viewControl.Clear();
  m_vecItems.Clear();
  m_viewControl.SetItems(m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::WaitForActionEvent(DWORD timeout)
{
  WaitForSingleObject(m_actionEvent, timeout);
  ResetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::PulseActionEvent()
{
  SetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::AllocResources(bool forceLoad /*= FALSE */)
{
  m_backupMediaDir = g_graphicsContext.GetMediaDir();
  CStdString tmpDir;
  CUtil::GetDirectory(m_xmlFile, tmpDir);
  if (!tmpDir.IsEmpty())
  {
    CStdString fallbackMediaPath;
    CUtil::GetParentPath(tmpDir, fallbackMediaPath);
    CUtil::RemoveSlashAtEnd(fallbackMediaPath);
    g_graphicsContext.SetMediaDir(fallbackMediaPath);
    m_fallbackPath = fallbackMediaPath;
    //CLog::Log(LOGDEBUG, "CGUIPythonWindowXML::AllocResources called: %s", fallbackMediaPath.c_str());
  }
  CGUIWindow::AllocResources(forceLoad);
  g_graphicsContext.SetMediaDir(m_backupMediaDir);
}

void CGUIPythonWindowXML::Render()
{
  g_graphicsContext.SetMediaDir(m_fallbackPath);
  CGUIWindow::Render();
  g_graphicsContext.SetMediaDir(m_backupMediaDir);
}

int Py_XBMC_Event_OnClick(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject_CallMethod(action->pCallbackWindow, "onClick", "i", action->controlId);
    delete action;
  }
  return 0;
}

int Py_XBMC_Event_OnFocus(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject_CallMethod(action->pCallbackWindow, "onFocus", "i", action->controlId);
    delete action;
  }
  return 0;
}

int Py_XBMC_Event_OnInit(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject_CallMethod(action->pCallbackWindow, "onInit", ""); //, "O", &self);
    delete action;
  }
  return 0;
}

/// Functions Below here are speceifc for the 'MediaWindow' Like stuff (such as Sort and View)

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIPythonWindowXML::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
}

// \brief Formats item labels based on the formatting provided by guiViewState
void CGUIPythonWindowXML::FormatItemLabels()
{
  if (!m_guiState.get())
    return;

  LABEL_MASKS labelMasks;
  m_guiState->GetSortMethodLabelMasks(labelMasks);
}
// \brief Sorts Fileitems based on the sort method and sort oder provided by guiViewState
void CGUIPythonWindowXML::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    items.Sort(guiState->GetSortMethod(), guiState->GetDisplaySortOrder());

    // Should these items be saved to the hdd
    if (items.GetCacheToDisc())
      items.Save();
  }
}
// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIPythonWindowXML::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[nItem];
  const CStdString& strSelected = pItem->m_strPath;

  OnSort();
  UpdateButtons();

  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetSelectedItem(strSelected);
}

// \brief Updates the states (enable, disable, visible...)
// of the controls defined by this window
// Override this function in a derived class to add new controls
void CGUIPythonWindowXML::UpdateButtons()
{
  if (m_guiState.get())
  {
    // Update sorting controls
    if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (m_guiState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    CStdString sortLabel;
    sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, sortLabel);
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIPythonWindowXML::OnInitWindow()
{
  // Update list/thumb control
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  m_viewControl.SetFocused();
  SET_CONTROL_VISIBLE(CONTROL_LIST);
  CGUIWindow::OnInitWindow();
}

CGUIControl *CGUIPythonWindowXML::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}
