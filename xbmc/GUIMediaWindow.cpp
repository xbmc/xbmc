#include "stdafx.h"
#include "GUIMediaWindow.h"
#include "util.h"
#include "detectdvdtype.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIG_LIST          52
#define CONTROL_LABELFILES        12


CGUIMediaWindow::CGUIMediaWindow(DWORD id, const char *xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_vecItems.m_strPath = "?";
  m_vecItems.m_bIsFolder = true;
  m_iLastControl = -1;
  m_iSelectedItem = -1;
}

CGUIMediaWindow::~CGUIMediaWindow()
{
}

bool CGUIMediaWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIMediaWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();
      ClearFileItems();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
        if (pState.get())
          while (!m_viewControl.HasViewMode(pState->SetNextViewAsControl()));

        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
        if (pState.get())
          pState->SetNextSortOrder();
        UpdateButtons();
        OnSort();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
        if (pState.get())
          pState->SetNextSortMethod();
        UpdateButtons();
        OnSort();
        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if this window is inactive
      
      //  Is there a dvd share in this window?
      if (!m_rootDir.GetDVDDriveUrl().IsEmpty())
      {
        if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
        {
          if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_vecItems.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }
          else if (m_vecItems.IsCDDA() || m_vecItems.IsOnDVD())
          { // Disc has changed and we are inside a DVD Drive share, get out of here :)
            if (IsActive()) Update("");
            else m_vecItems.m_strPath="";
          }

          return true;
        }
        else if (message.GetParam1()==GUI_MSG_DVDDRIVE_CHANGED_CD)
        { // State of the dvd-drive changed (Open/Busy label,...), so update it
          if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_vecItems.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }

          return true;
        }
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIMediaWindow::UpdateButtons()
{
  auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
  if (pState.get())
  {
    // Update sorting controls
    if (pState->GetSortOrder()==SORT_ORDER_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (pState->GetSortOrder()==SORT_ORDER_ASC)
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
    m_viewControl.SetCurrentView(pState->GetViewAsControl());

    // Update sort by button
    if (pState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, pState->GetSortMethodLabel());
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);
  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);
}

void CGUIMediaWindow::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

void CGUIMediaWindow::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), items));

  if (pState.get() && 
      (pState->GetSortMethod()!=items.GetSortMethod() || 
       pState->GetSortOrder()!=items.GetSortOrder()))
  {
    items.Sort(pState->GetSortMethod(), pState->GetSortOrder());
  }
}

void CGUIMediaWindow::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);
}

bool CGUIMediaWindow::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      strSelectedItem = pItem->m_strPath;
      m_history.Set(strSelectedItem, m_vecItems.m_strPath);
    }
  }
  ClearFileItems();

  CStdString strParentPath;
  bool bParentExists = false;
  if (strDirectory != "Q:\\scripts")
    bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        m_vecItems.Add(pItem);
      }
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = "";
  }

  m_rootDir.GetDirectory(strDirectory, m_vecItems);
  m_vecItems.SetThumbs();
  if (g_guiSettings.GetBool("FileLists.HideExtensions"))
    m_vecItems.RemoveExtensions();

  m_vecItems.FillInDefaultIcons();

  m_iLastControl = GetFocusedControl();
  OnSort();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_vecItems.m_strPath);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
  return true;
}

void CGUIMediaWindow::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    CStdString directory = pItem->m_strPath;
    Update(directory);
  }
}

bool CGUIMediaWindow::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    MEDIA_DETECT::CDetectDVDMedia::WaitMediaReady();
    if ( !MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      return false;
    }
  }
  else if ( iDriveType == SHARE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }
  else
    return true;
  return true;
}

void CGUIMediaWindow::GoParentFolder()
{
  CStdString strPath(m_strParentPath), strOldPath(m_vecItems.m_strPath);
  Update(strPath);
  UpdateButtons();

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path

}

void CGUIMediaWindow::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_METHOD_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_METHOD_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_LIST, GetControl(CONTROL_BIG_LIST));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

void CGUIMediaWindow::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

const CFileItem *CGUIMediaWindow::GetCurrentListItem() const
{
  int iItem = m_viewControl.GetSelectedItem();
  if (iItem < 0) return NULL;
  return m_vecItems[iItem];
}
