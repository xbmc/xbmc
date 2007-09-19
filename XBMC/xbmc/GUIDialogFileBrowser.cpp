/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogFileBrowser.h"
#include "Util.h"
#include "DetectDVDType.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogContextMenu.h"
#include "MediaManager.h"
#include "AutoSwitch.h"
#include "utils/Network.h"
#include "GUIPassword.h"

using namespace XFILE;
using namespace DIRECTORY;

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
    if ((!m_addSourceType.IsEmpty() && iItem != m_vecItems.Size()-1) || (m_addNetworkShareEnabled && g_mediaManager.HasLocation(m_selectedPath)))
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
      bool bIsDir = false;
      // this code allows two different selection modes for directories
      // end the path with a slash to start inside the directory
      if (CUtil::HasSlashAtEnd(m_selectedPath))
      {
        bIsDir = true;
        bool bFool;
        int iSource = CUtil::GetMatchingShare(m_selectedPath,m_shares,bFool);
        bFool = true;
        if (iSource > -1)
        {
          if (m_shares[iSource].strPath.Equals(m_selectedPath))
            bFool = false;
        }
      
        if (bFool && !CDirectory::Exists(m_selectedPath))
          m_selectedPath.Empty();
      }
      else
      {
        if (!CFile::Exists(m_selectedPath) && !CDirectory::Exists(m_selectedPath))
            m_selectedPath.Empty();
      }

      // find the parent folder if we are a file browser (don't do this for folders)
      m_Directory.m_strPath = m_selectedPath;
      if (!m_browsingForFolders && !bIsDir)
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
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        if (m_Directory.IsVirtualDirectoryRoot() && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Update(m_Directory.m_strPath);
          m_viewControl.SetSelectedItem(iItem);
        }
        else if (m_Directory.IsRemovable())
        { // check that we have this removable share still
          if (!m_rootDir.IsInShare(m_Directory.m_strPath))
          { // don't have this share any more
            if (IsActive()) Update("");
            else
            {
              m_history.ClearPathHistory();
              m_Directory.m_strPath="";
            }
          }
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_SOURCES)
      { // State of the sources changed, so update our view
        if (m_Directory.IsVirtualDirectoryRoot() && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Update(m_Directory.m_strPath);
          m_viewControl.SetSelectedItem(iItem);
        }
        return true;
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
  if (m_Directory.m_strPath.IsEmpty() && !m_addSourceType.IsEmpty())
  {
    CFileItem *pItem = new CFileItem(g_localizeStrings.Get(21359));
    pItem->m_strPath = "source://";
    pItem->m_bIsFolder = true;
    m_vecItems.Add(pItem);
  }

  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetCurrentView((m_browsingForImages && CAutoSwitch::ByFileCount(m_vecItems)) ? DEFAULT_VIEW_ICONS : DEFAULT_VIEW_LIST);

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
    if ((!m_browsingForFolders && m_vecItems[item]->m_bIsFolder) || m_viewControl.GetCurrentControl() == CONTROL_THUMBS)
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
    if (pItem->m_strPath == "source://")
    { // special "Add Source" item
      OnAddMediaSource();
      return;
    }
    if (!m_addSourceType.IsEmpty())
    {
      OnEditMediaSource(pItem);
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
    if ( !g_network.IsConnected() )
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
  if (strPath.size() == 2)
    if (strPath[1] == ':')
      CUtil::AddSlashAtEnd(strPath);
  Update(strPath);

  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
    m_history.RemoveSelectedItem(strOldPath); //Delete current path
}

void CGUIDialogFileBrowser::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
  m_viewControl.AddView(GetControl(CONTROL_THUMBS));
  // set the page spin controls to hidden
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIControl *spin = (CGUIControl *)GetControl(CONTROL_LIST + 5000);
  if (spin) spin->SetVisible(false);
  spin = (CGUIControl *)GetControl(CONTROL_THUMBS + 5000);
  if (spin) spin->SetVisible(false);
#endif
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
    item->SetLabel(g_localizeStrings.Get(20153));
    item->SetThumbnailImage("defaultPictureBig.png");
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
      return ShowAndGetImage(shares, g_localizeStrings.Get(21371), result);
    }
  }

  m_gWindowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}

bool CGUIDialogFileBrowser::ShowAndGetImage(const VECSHARES &shares, const CStdString &heading, CStdString &path)
{
  return ShowAndGetFile(shares, ".png|.jpg|.bmp|.gif|.tbn", heading, path, true); // true for use thumbs
}

bool CGUIDialogFileBrowser::ShowAndGetDirectory(const VECSHARES &shares, const CStdString &heading, CStdString &path, bool bWriteOnly)
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

bool CGUIDialogFileBrowser::ShowAndGetFile(const VECSHARES &shares, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs /* = false */, bool useFileDirectories /* = false */)
{
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser)
    return false;
  m_gWindowManager.AddUniqueInstance(browser);

  browser->m_useFileDirectories = useFileDirectories;

  browser->m_browsingForImages = useThumbs;
  browser->SetHeading(heading);
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

// same as above, starting in a single directory
bool CGUIDialogFileBrowser::ShowAndGetFile(const CStdString &directory, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs /* = false */, bool useFileDirectories /* = false */)
{
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser)
    return false;
  m_gWindowManager.AddUniqueInstance(browser);

  browser->m_useFileDirectories = useFileDirectories;

  // add a single share for this directory
  VECSHARES shares;
  CShare share;
  share.strPath = directory;
  CUtil::RemoveSlashAtEnd(share.strPath); // this is needed for the dodgy code in WINDOW_INIT
  shares.push_back(share);
  browser->m_browsingForImages = useThumbs;
  browser->SetHeading(heading);
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
  browser->m_selectedPath = directory;
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

bool CGUIDialogFileBrowser::ShowAndGetShare(CStdString &path, bool allowNetworkShares, VECSHARES* additionalShare /* = NULL */, const CStdString& strType /* = "" */)
{
  // Technique is
  // 1.  Show Filebrowser with currently defined local, and optionally the network locations.
  // 2.  Have the "Add Network Location" option in addition.
  // 3a. If the "Add Network Location" is pressed, then:
  //     a) Fire up the network location dialog to grab the new location
  //     b) Check the location by doing a GetDirectory() - if it fails, prompt the user
  //        to allow them to add currently disconnected network shares.
  //     c) Save this location to our xml file (network.xml)
  //     d) Return to 1.
  // 3b. If the "Add Source" is pressed, then:
  //     a) Fire up the media source dialog to add the new location
  // 4.  Optionally allow user to browse the local and network locations for their share.
  // 5.  On OK, return.

  // Create a new filebrowser window
  CGUIDialogFileBrowser *browser = new CGUIDialogFileBrowser();
  if (!browser) return false;

  // Add it to our window manager
  m_gWindowManager.AddUniqueInstance(browser);

  VECSHARES shares;
  if (!strType.IsEmpty())
  {
    if (strType.Equals("upnpmusic"))
      browser->SetHeading(g_localizeStrings.Get(21361));
    if (strType.Equals("upnpvideo"))
      browser->SetHeading(g_localizeStrings.Get(21362));
    if (strType.Equals("upnppictures"))
      browser->SetHeading(g_localizeStrings.Get(21363));
    if (additionalShare)
      shares = *additionalShare;
    browser->m_addSourceType = strType;
  }
  else
  {
    browser->SetHeading(g_localizeStrings.Get(1023));

    g_mediaManager.GetLocalDrives(shares);

    // Now the additional share if appropriate
    if (additionalShare)
    {
      shares.insert(shares.end(),additionalShare->begin(),additionalShare->end());
    }

    // Now add the network shares...
    if (allowNetworkShares)
    {
      g_mediaManager.GetNetworkLocations(shares);
    }
  }
  
  browser->SetShares(shares);
  browser->m_rootDir.SetMask("/");
  browser->m_rootDir.AllowNonLocalShares(false);  // don't allow plug n play shares
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

void CGUIDialogFileBrowser::SetShares(const VECSHARES &shares)
{
  m_shares = shares;
  if (!m_shares.size() && m_addSourceType.IsEmpty())
    g_mediaManager.GetLocalDrives(m_shares);
  m_rootDir.SetShares(m_shares);
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

void CGUIDialogFileBrowser::OnAddMediaSource()
{
  if (CGUIDialogMediaSource::ShowAndAddMediaSource(m_addSourceType))
  {
    SetShares(*g_settings.GetSharesFromType(m_addSourceType));
    Update("");
  }
}

void CGUIDialogFileBrowser::OnEditMediaSource(CFileItem* pItem)
{
  if (CGUIDialogMediaSource::ShowAndEditMediaSource(m_addSourceType,pItem->GetLabel()))
  {
    SetShares(*g_settings.GetSharesFromType(m_addSourceType));
    Update("");
  }
}

bool CGUIDialogFileBrowser::OnPopupMenu(int iItem)
{
  CGUIDialogContextMenu* pMenu = (CGUIDialogContextMenu*)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu)
    return false;

  float posX = 200, posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  pMenu->Initialize();

  int iEditLabel = 20133;
  int iRemoveLabel = 20134;
  if (!m_addSourceType.IsEmpty())
  {
    iEditLabel = 21364;
    iRemoveLabel = 21365;
  }
  int btn_Edit = pMenu->AddButton(iEditLabel);
  int btn_Remove = pMenu->AddButton(iRemoveLabel);

  pMenu->SetPosition(posX, posY);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid == btn_Edit)
  {
    if (m_addNetworkShareEnabled)
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
    else
      OnEditMediaSource(m_vecItems[iItem]);
  }
  if (btnid == btn_Remove)
  {
    if (m_addNetworkShareEnabled)
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
    else
    {
      g_settings.DeleteSource(m_addSourceType,m_vecItems[iItem]->GetLabel(),m_vecItems[iItem]->m_strPath);
      SetShares(*g_settings.GetSharesFromType(m_addSourceType));
      Update("");
    }
  }

  return true;
}

CFileItem *CGUIDialogFileBrowser::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (item < 0 || !m_vecItems.Size()) return NULL;

  item = (item + offset) % m_vecItems.Size();
  if (item < 0) item += m_vecItems.Size();
  return m_vecItems[item];
}

CGUIControl *CGUIDialogFileBrowser::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}

