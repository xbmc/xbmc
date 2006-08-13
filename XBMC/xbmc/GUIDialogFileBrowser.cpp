#include "stdafx.h"
#include "GUIDialogFileBrowser.h"
#include "util.h"
#include "detectdvdtype.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIListControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIDialogContextMenu.h"
#include "MediaManager.h"
#include "AutoSwitch.h"
#include "xbox/network.h"

#define CONTROL_LIST          450
#define CONTROL_THUMBS        451
#define CONTROL_HEADING_LABEL 411
#define CONTROL_LABEL_PATH    412
#define CONTROL_OK            413
#define CONTROL_CANCEL        414
#define CONTROL_NEWFOLDER     415

CGUIDialogFileBrowser::CGUIDialogFileBrowser()
    : CGUIDialog(WINDOW_DIALOG_FILE_BROWSER, "FileBrowser.xml")
{
  m_bConfirmed = false;
  m_Directory.m_bIsFolder = true;
  m_browsingForFolders = 0;
  m_browsingForImages = false;
  m_addNetworkShareEnabled = false;
  m_singleList = false;
  m_thumbLoader.SetObserver(this);
}

CGUIDialogFileBrowser::~CGUIDialogFileBrowser()
{
}

bool CGUIDialogFileBrowser::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }
  if ((action.wID == ACTION_CONTEXT_MENU || action.wID == ACTION_MOUSE_RIGHT_CLICK) && m_Directory.m_strPath.IsEmpty())
  {
    int iItem = m_viewControl.GetSelectedItem();  
    if (g_mediaManager.HasLocation(m_selectedPath))
      return OnPopupMenu(iItem);
    
    return false;
  }
  
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogFileBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      CGUIDialog::OnMessage(message);
      ClearFileItems();
      m_addNetworkShareEnabled = false;
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      if (!CFile::Exists(m_selectedPath))
        m_selectedPath.Empty();

      // find the parent folder if we are a file browser (don't do this for folders)
      m_Directory.m_strPath = m_selectedPath;
      if (!m_browsingForFolders)
        CUtil::GetParentPath(m_selectedPath, m_Directory.m_strPath);
      Update(m_Directory.m_strPath);
      m_viewControl.SetSelectedItem(m_selectedPath);
      return CGUIDialog::OnMessage(message);
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
          return true;
        }
      }
      else if (message.GetSenderId() == CONTROL_OK)
      {
        if (m_browsingForFolders == 2)
        {
          CStdString strTest;
          int iItem = m_viewControl.GetSelectedItem();

          CStdString strPath;
          if (iItem == 0)
            strPath = m_selectedPath;
          else
            strPath = m_vecItems[iItem]->m_strPath;

          CUtil::AddFileToFolder(strPath,"1",strTest);
          CFile file;
          if (file.OpenForWrite(strTest,true,true))
          {
            file.Close();
            CFile::Delete(strTest);
            m_bConfirmed = true;
            Close();
          }
          else
            CGUIDialogOK::ShowAndGetInput(257,20072,0,0);
        }
        else
        {
          m_bConfirmed = true;
          Close();
        }
        return true;
      }
      else if (message.GetSenderId() == CONTROL_CANCEL)
      {
        Close();
        return true;
      }
      else if (message.GetSenderId() == CONTROL_NEWFOLDER)
      {
        CStdString strInput;
        if (CGUIDialogKeyboard::ShowAndGetInput(strInput,g_localizeStrings.Get(119),false))
        {
          CStdString strPath;
          CUtil::AddFileToFolder(m_vecItems.m_strPath,strInput,strPath);
          if (CDirectory::Create(strPath))
            Update(m_vecItems.m_strPath);
          else
            CGUIDialogOK::ShowAndGetInput(20069,20072,20073,0);
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
  if (!m_singleList)
    m_vecItems.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}

void CGUIDialogFileBrowser::Update(const CStdString &strDirectory)
{
  if (m_browsingForImages && m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      strSelectedItem = pItem->m_strPath;
      CUtil::RemoveSlashAtEnd(strSelectedItem);
      m_history.SetSelectedItem(strSelectedItem, m_Directory.m_strPath==""?"empty":m_Directory.m_strPath);
    }
  }

  if (!m_singleList)
  {
    ClearFileItems();

    CStdString strParentPath;
    bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

    // check if current directory is a root share
/*    if (!g_guiSettings.GetBool("filelists.hideparentdiritems"))
    {*/
      if ( !m_rootDir.IsShare(strDirectory))
      {
        // no, do we got a parent dir?
        if (bParentExists)
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
    //}
    m_Directory.m_strPath = strDirectory;
    m_rootDir.GetDirectory(strDirectory, m_vecItems,m_useFileDirectories);
  }

  // some evil stuff don't work with the '/' mask, e.g. shoutcast directory - make sure no files are in there
  if (m_browsingForFolders)
  {
    for (int i=0;i<m_vecItems.Size();++i)
      if (!m_vecItems[i]->m_bIsFolder)
      {
        m_vecItems.Remove(i);
        i--;
      }
  }

  // No need to set thumbs

  m_vecItems.FillInDefaultIcons();

  OnSort();

  if (m_Directory.m_strPath.IsEmpty() && m_addNetworkShareEnabled && (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || (g_settings.m_iLastLoadedProfileIndex == 0) || g_passwordManager.bMasterUser))
  { // we are in the virtual directory - add the "Add Network Location" item
    CFileItem *pItem = new CFileItem(g_localizeStrings.Get(1032));
    pItem->m_strPath = "net://";
    pItem->m_bIsFolder = true;
    m_vecItems.Add(pItem);
  }

  m_viewControl.SetItems(m_vecItems);
  if (m_browsingForImages)
    m_viewControl.SetCurrentView(CAutoSwitch::ByFileCount(m_vecItems) ? VIEW_METHOD_ICONS : VIEW_METHOD_LIST);

  CStdString strPath2 = m_Directory.m_strPath;
  CUtil::RemoveSlashAtEnd(strPath2);
  strSelectedItem = m_history.GetSelectedItem(strPath2==""?"empty":strPath2);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    strPath2 = pItem->m_strPath;
    CUtil::RemoveSlashAtEnd(strPath2);
    if (strPath2 == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
  if (m_browsingForImages)
    m_thumbLoader.Load(m_vecItems);
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
      SET_CONTROL_LABEL(CONTROL_LABEL_PATH, g_localizeStrings.Get(1032)); // "Add Network Location..."
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
    if (m_browsingForFolders == 2)
    {
      CONTROL_ENABLE(CONTROL_NEWFOLDER);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_NEWFOLDER);
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
    if ( !g_network.IsEthernetConnected() )
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

  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
    m_history.RemoveSelectedItem(strOldPath); //Delete current path
}

void CGUIDialogFileBrowser::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_METHOD_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_METHOD_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.SetCurrentView(VIEW_METHOD_LIST);
  // set the page spin controls to hidden
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList) pList->SetPageControlVisible(false);
  CGUIThumbnailPanel *pThumbs = (CGUIThumbnailPanel *)GetControl(CONTROL_THUMBS);
  if (pThumbs) pThumbs->SetPageControlVisible(false);
}

void CGUIDialogFileBrowser::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

bool CGUIDialogFileBrowser::ShowAndGetImage(const CFileItemList &items, VECSHARES &shares, const CStdString &heading, CStdString &result)
{
  CStdString mask = ".png|.jpg|.bmp|.gif";
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser)
    return false;
  m_gWindowManager.AddUniqueInstance(browser);

  browser->m_browsingForImages = true;
  browser->m_singleList = true;
  browser->m_vecItems.Clear();
  browser->m_vecItems.Append(items);
  if (true)
  {
    CFileItem *item = new CFileItem("image://Browse", false);
    item->SetLabel("Browse...");
    browser->m_vecItems.Add(item);
  }
  browser->SetHeading(heading);
  browser->DoModal();
  bool confirmed(browser->IsConfirmed());
  if (confirmed)
  {
    result = browser->m_selectedPath;
    if (result == "image://Browse")
    { // "Browse for thumb"
      m_gWindowManager.Remove(browser->GetID());
      delete browser;
      return ShowAndGetImage(shares, heading, result);
    }
  }

  m_gWindowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}

bool CGUIDialogFileBrowser::ShowAndGetImage(VECSHARES &shares, const CStdString &heading, CStdString &path)
{
  return ShowAndGetFile(shares, ".png|.jpg|.bmp|.gif", heading, path, true); // true for use thumbs
}

bool CGUIDialogFileBrowser::ShowAndGetDirectory(VECSHARES &shares, const CStdString &heading, CStdString &path, bool bWriteOnly)
{
  // an extension mask of "/" ensures that no files are shown
  if (bWriteOnly)
  {
    VECSHARES shareWritable;
    for (unsigned int i=0;i<shares.size();++i)
    {
      if (shares[i].isWritable())
        shareWritable.push_back(shares[i]);
    }

    return ShowAndGetFile(shareWritable, "/w", heading, path);
  }

  return ShowAndGetFile(shares, "/", heading, path);
}

bool CGUIDialogFileBrowser::ShowAndGetFile(VECSHARES &shares, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs /* = false */, bool useFileDirectories /* = false */)
{
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser)
    return false;
  m_gWindowManager.AddUniqueInstance(browser);

  browser->m_useFileDirectories = useFileDirectories;

  CStdString browseHeading;
  browseHeading.Format(g_localizeStrings.Get(13401).c_str(), heading.c_str());
  browser->m_browsingForImages = useThumbs;
  browser->SetHeading(browseHeading);
  browser->SetShares(shares);
  CStdString strMask = mask;
  if (mask == "/")
    browser->m_browsingForFolders=1;
  else
  if (mask == "/w")
  {
    browser->m_browsingForFolders=2;
    strMask = "/";
  }
  else
    browser->m_browsingForFolders = 0;

  browser->m_rootDir.SetMask(strMask);
  browser->m_selectedPath = path;
  browser->m_addNetworkShareEnabled = false;
  browser->DoModal();
  bool confirmed(browser->IsConfirmed());
  if (confirmed)
    path = browser->m_selectedPath;
  m_gWindowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}

void CGUIDialogFileBrowser::SetHeading(const CStdString &heading)
{
  Initialize();
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, heading);
}

bool CGUIDialogFileBrowser::ShowAndGetShare(CStdString &path, bool allowNetworkShares, VECSHARES* additionalShare /* = NULL */)
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

  // Create a new filebrowser window
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser) return false;

  // Add it to our window manager
  m_gWindowManager.AddUniqueInstance(browser);

  browser->SetHeading(g_localizeStrings.Get(1023));

  VECSHARES shares;
  g_mediaManager.GetLocalDrives(shares);

  // Now the additional share if appropriate
  if (additionalShare)
  {
    for (unsigned int i=0;i<additionalShare->size();++i)
    shares.push_back((*additionalShare)[i]);
  }

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
  browser->DoModal();
  bool confirmed = browser->IsConfirmed();
  if (confirmed)
    path = browser->m_selectedPath;

  m_gWindowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}

void CGUIDialogFileBrowser::SetShares(VECSHARES &shares)
{
  m_shares = shares;
  m_rootDir.SetShares(shares);
}

void CGUIDialogFileBrowser::OnAddNetworkLocation()
{
  // ok, fire up the network location dialog
  CStdString path;
  if (CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(path))
  {
    // verify the path by doing a GetDirectory.
    CFileItemList items;
    if (CDirectory::GetDirectory(path, items, "", false, true) || CGUIDialogYesNo::ShowAndGetInput(1001,1002,1003,1004))
    { // add the network location to the shares list
      CShare share;
      share.strPath = path; //setPath(path);
      CURL url(path);
      url.GetURLWithoutUserDetails(share.strName);
      m_shares.push_back(share);
      // add to our location manager...
      g_mediaManager.AddNetworkLocation(path);
    }
  }
  m_rootDir.SetShares(m_shares);
  Update(m_vecItems.m_strPath);
}

bool CGUIDialogFileBrowser::OnPopupMenu(int iItem)
{
  CGUIDialogContextMenu* pMenu = (CGUIDialogContextMenu*)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu)
    return false;
  
  int iPosX = 200, iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  pMenu->Initialize();
  
  int btn_Edit = pMenu->AddButton(20133);
  int btn_Remove = pMenu->AddButton(20134);

  pMenu->SetPosition(iPosX, iPosY);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid == btn_Edit)
  {
    CStdString strOldPath=m_selectedPath,newPath=m_selectedPath;
    VECSHARES shares=m_shares;
    if (CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(newPath))
    {
      g_mediaManager.SetLocationPath(strOldPath,newPath);
      for (unsigned int i=0;i<shares.size();++i)
      {
        if (shares[i].strPath.Equals(strOldPath))//getPath().Equals(strOldPath))
        {
          shares[i].strName = newPath;
          shares[i].strPath = newPath;//setPath(newPath);
          break;
        }
      }
      // re-open our dialog
      SetShares(shares);
      m_rootDir.SetMask("/");
      m_browsingForFolders = true;
      m_addNetworkShareEnabled = true;
      m_selectedPath = newPath;
      DoModal();    
    }
  }
  if (btnid == btn_Remove)
  {
    g_mediaManager.RemoveLocation(m_selectedPath);
    
    for (unsigned int i=0;i<m_shares.size();++i)
    {
      if (m_shares[i].strPath.Equals(m_selectedPath)) // getPath().Equals(m_selectedPath))
      {
        m_shares.erase(m_shares.begin()+i);
        break;
      }
    }
    m_rootDir.SetShares(m_shares);
    m_rootDir.SetMask("/");
    m_browsingForFolders = true;
    m_addNetworkShareEnabled = true;
    m_selectedPath = "";

    Update(m_Directory.m_strPath);
  }
  
  return true;
}

const CFileItem *CGUIDialogFileBrowser::GetCurrentListItem() const
{
  int iItem = m_viewControl.GetSelectedItem();
  if (iItem < 0) return NULL;
  return m_vecItems[iItem];
}

CGUIControl *CGUIDialogFileBrowser::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}