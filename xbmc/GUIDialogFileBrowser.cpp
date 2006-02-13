#include "stdafx.h"
#include "GUIDialogFileBrowser.h"
#include "util.h"
#include "detectdvdtype.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIListControl.h"
#include "MediaManager.h"

#define CONTROL_LIST          450
#define CONTROL_HEADING_LABEL 411
#define CONTROL_LABEL_PATH    412
#define CONTROL_OK            413
#define CONTROL_CANCEL        414

CGUIDialogFileBrowser::CGUIDialogFileBrowser()
    : CGUIDialog(WINDOW_DIALOG_FILE_BROWSER, "FileBrowser.xml")
{
  m_bConfirmed = false;
  m_Directory.m_bIsFolder = true;
  m_browsingForFolders = false;
  m_addNetworkShareEnabled = false;
}

CGUIDialogFileBrowser::~CGUIDialogFileBrowser()
{}

bool CGUIDialogFileBrowser::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogFileBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearFileItems();
      m_addNetworkShareEnabled = false;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bConfirmed = false;

      if (!CFile::Exists(m_selectedPath))
        m_selectedPath.Empty();

      // find the parent folder
      m_Directory.m_strPath = m_selectedPath;
//      CUtil::GetParentPath(m_selectedPath, m_Directory.m_strPath);
      Update(m_Directory.m_strPath);
      m_viewControl.SetSelectedItem(m_selectedPath);
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      if (m_viewControl.HasControl(message.GetSenderId()))  // list control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
      }
      else if (message.GetSenderId() == CONTROL_OK)
      {
        m_bConfirmed = true;
        Close();
      }
      else if (message.GetSenderId() == CONTROL_CANCEL)
      {
        Close();
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
    { // Message is received only if this window is active
      
      //  Is there a dvd share in this window?
      if (!m_rootDir.GetDVDDriveUrl().IsEmpty())
      {
        if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
        {
          if (m_Directory.IsVirtualDirectoryRoot())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_Directory.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }
          else if (m_Directory.IsCDDA() || m_Directory.IsOnDVD())
          { // Disc has changed and we are inside a DVD Drive share, get out of here :)
            Update("");
          }

          return true;
        }
        else if (message.GetParam1()==GUI_MSG_DVDDRIVE_CHANGED_CD)
        { // State of the dvd-drive changed (Open/Busy label,...), so update it
          if (m_Directory.IsVirtualDirectoryRoot())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_Directory.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }

          return true;
        }
      }
    }
    break;

  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogFileBrowser::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

void CGUIDialogFileBrowser::OnSort()
{
  m_vecItems.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}

void CGUIDialogFileBrowser::Update(const CStdString &strDirectory)
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
      m_history.SetSelectedItem(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = strParentPath;
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    CFileItem *pItem = new CFileItem("..");
    pItem->m_strPath = "";
    pItem->m_bIsShareOrDrive = false;
    pItem->m_bIsFolder = true;
    m_vecItems.Add(pItem);
    m_strParentPath = "";
  }

  m_Directory.m_strPath = strDirectory;
  m_rootDir.GetDirectory(strDirectory, m_vecItems);

  m_vecItems.SetThumbs();

  m_vecItems.FillInDefaultIcons();

  OnSort();

  if (m_Directory.m_strPath.IsEmpty() && m_addNetworkShareEnabled)
  { // we are in the virtual directory - add the "Add Network Location" item
    CFileItem *pItem = new CFileItem("Add Network Location...");
    pItem->m_strPath = "net://";
    pItem->m_bIsFolder = true;
    m_vecItems.Add(pItem);
  }

  m_viewControl.SetItems(m_vecItems);

  strSelectedItem = m_history.GetSelectedItem(m_Directory.m_strPath);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
}

void CGUIDialogFileBrowser::Render()
{
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0)
  {
    // as we don't have a "." item, assume that if the user
    // is highlighting ".." then they wish to use that as the path
    if (m_vecItems[item]->IsParentFolder())
      m_selectedPath = m_Directory.m_strPath;
    else
      m_selectedPath = m_vecItems[item]->m_strPath;
    if (m_selectedPath == "net://")
    {
      SET_CONTROL_LABEL(CONTROL_LABEL_PATH, "Add Network Location...");
    }
    else
    {
      // Update the current path label
      CURL url(m_selectedPath);
      CStdString safePath;
      url.GetURLWithoutUserDetails(safePath);
      SET_CONTROL_LABEL(CONTROL_LABEL_PATH, safePath);
    }
    if (!m_browsingForFolders && m_vecItems[item]->m_bIsFolder)
    {
      CONTROL_DISABLE(CONTROL_OK);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_OK);
    }
  }
  CGUIDialog::Render();
}

void CGUIDialogFileBrowser::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  if (pItem->m_bIsFolder)
  {
    if (pItem->m_strPath == "net://")
    { // special "Add Network Location" item
      OnAddNetworkLocation();
      return;
    }
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else if (!m_browsingForFolders)
  {
    m_selectedPath = pItem->m_strPath;
    m_bConfirmed = true;
    Close();
  }
}

bool CGUIDialogFileBrowser::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
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

void CGUIDialogFileBrowser::GoParentFolder()
{
  CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
  Update(strPath);

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.RemoveSelectedItem(strOldPath); //Delete current path
}

void CGUIDialogFileBrowser::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_METHOD_LIST, GetControl(CONTROL_LIST));
  m_viewControl.SetCurrentView(VIEW_METHOD_LIST);
  // set the page spin control to hidden
  CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pControl) pControl->SetPageControlVisible(false);
}

void CGUIDialogFileBrowser::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

bool CGUIDialogFileBrowser::ShowAndGetDirectory(VECSHARES &shares, const CStdStringW &heading, CStdString &path)
{
  // an extension mask of "/" ensures that no files are shown
  return ShowAndGetFile(shares, "/", heading, path);
}

bool CGUIDialogFileBrowser::ShowAndGetFile(VECSHARES &shares, const CStdString &mask, const CStdStringW &heading, CStdString &path)
{
  CGUIDialogFileBrowser *browser = (CGUIDialogFileBrowser *)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILE_BROWSER);
  if (!browser)
    return false;
  CStdStringW browseHeading;
  browseHeading.Format(g_localizeStrings.Get(13401).c_str(), heading.c_str());
  browser->SetHeading(browseHeading);
  browser->SetShares(shares);
  browser->m_rootDir.SetMask(mask);
  browser->m_browsingForFolders = (mask == "/");
  browser->m_selectedPath = path;
  browser->DoModal(m_gWindowManager.GetActiveWindow());
  if (browser->IsConfirmed())
  {
    path = browser->m_selectedPath;
    return true;
  }
  return false;
}

void CGUIDialogFileBrowser::SetHeading(const CStdStringW &heading)
{
  Initialize();
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, heading);
}

bool CGUIDialogFileBrowser::ShowAndGetShare(CStdString &path, bool allowNetworkShares)
{
  // Technique is
  // 1.  Show Filebrowser with currently defined local, and optionally the network locations.
  // 2.  Have the "Add Network Location" option in addition.
  // 3.  If the "Add Network Location" is pressed, then:
  //     a) Fire up the network location dialog to grab the new location
  //     b) Check the location by doing a GetDirectory() - if it fails, prompt the user
  //        to allow them to add currently disconnected network shares.
  //     c) Save this location to our xml file (network.xml)
  //     d) Return to 1.
  // 4.  Allow user to browse the local and network locations for their share.
  // 5.  On OK, return to the Add share dialog.
  CGUIDialogFileBrowser *browser = (CGUIDialogFileBrowser *)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILE_BROWSER);
  if (!browser) return false;

  browser->SetHeading(g_localizeStrings.Get(1023));

  VECSHARES shares;
  g_mediaManager.GetLocalDrives(shares);

  // Now add the network shares...
  if (allowNetworkShares)
  {
    g_mediaManager.GetNetworkLocations(shares);
  }
  browser->SetShares(shares);
  browser->m_rootDir.SetMask("/");
  browser->m_browsingForFolders = true;
  browser->m_addNetworkShareEnabled = allowNetworkShares;
  browser->m_selectedPath = "";
  browser->DoModal(m_gWindowManager.GetActiveWindow());
  if (browser->IsConfirmed())
  {
    path = browser->m_selectedPath;
    return true;
  }
  return false;
}

void CGUIDialogFileBrowser::SetShares(VECSHARES &shares)
{
  m_shares = shares;
  m_rootDir.SetShares(shares);
}

void CGUIDialogFileBrowser::OnAddNetworkLocation()
{
  // Close the current dialog as it will be reused by this method
  VECSHARES shares = m_shares;
  Close();

  // ok, fire up the network location dialog
  CStdString path;
  if (CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(path))
  {
    // verify the path by doing a GetDirectory.
    CFileItemList items;
    if (CDirectory::GetDirectory(path, items, "", false, true) || CGUIDialogYesNo::ShowAndGetInput(1001,1002,1003,1004))
    { // add the network location to the shares list
      CShare share;
      share.strPath = path;
      CURL url(path);
      url.GetURLWithoutUserDetails(share.strName);
      shares.push_back(share);
      // add to our location manager...
      g_mediaManager.AddNetworkLocation(path);
    }
  }

  // re-open our dialog
  SetShares(shares);
  m_rootDir.SetMask("/");
  m_browsingForFolders = true;
  m_addNetworkShareEnabled = true;
  m_selectedPath = "";
  DoModal(m_gWindowManager.GetActiveWindow());
}