/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "GUIWindowFileManager.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/ZipManager.h"
#include "filesystem/FileDirectoryFactory.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "playlists/PlayListFactory.h"
#include "network/Network.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/FavouritesDirectory.h"
#include "playlists/PlayList.h"
#include "storage/MediaManager.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "input/InputManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "Autorun.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;

#define ACTION_COPY                     1
#define ACTION_MOVE                     2
#define ACTION_DELETE                   3
#define ACTION_CREATEFOLDER             4
#define ACTION_DELETEFOLDER             5

#define CONTROL_BTNVIEWASICONS          2
#define CONTROL_BTNSORTBY               3
#define CONTROL_BTNSORTASC              4
#define CONTROL_BTNNEWFOLDER            6
#define CONTROL_BTNSELECTALL            7
#define CONTROL_BTNCOPY                10
#define CONTROL_BTNMOVE                11
#define CONTROL_BTNDELETE               8
#define CONTROL_BTNRENAME               9

#define CONTROL_NUMFILES_LEFT          12
#define CONTROL_NUMFILES_RIGHT         13

#define CONTROL_LEFT_LIST              20
#define CONTROL_RIGHT_LIST             21

#define CONTROL_CURRENTDIRLABEL_LEFT  101
#define CONTROL_CURRENTDIRLABEL_RIGHT 102

CGUIWindowFileManager::CGUIWindowFileManager(void)
    : CGUIWindow(WINDOW_FILES, "FileManager.xml"),
      CJobQueue(false,2)
{
  m_Directory[0] = new CFileItem;
  m_Directory[1] = new CFileItem;
  m_vecItems[0] = new CFileItemList;
  m_vecItems[1] = new CFileItemList;
  m_Directory[0]->SetPath("?");
  m_Directory[1]->SetPath("?");
  m_Directory[0]->m_bIsFolder = true;
  m_Directory[1]->m_bIsFolder = true;
  bCheckShareConnectivity = true;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowFileManager::~CGUIWindowFileManager(void)
{
  delete m_Directory[0];
  delete m_Directory[1];
  delete m_vecItems[0];
  delete m_vecItems[1];
}

bool CGUIWindowFileManager::OnAction(const CAction &action)
{
  int list = GetFocusedList();
  if (list >= 0 && list <= 1)
  {
    int item;

    // the non-contextual menu can be called at any time
    if (action.GetID() == ACTION_CONTEXT_MENU && m_vecItems[list]->Size() == 0)
    {
      OnPopupMenu(list,-1, false);
      return true;
    }
    if (action.GetID() == ACTION_DELETE_ITEM)
    {
      if (CanDelete(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnDelete(list);
        if (bDeselect) m_vecItems[list]->Get(item)->Select(false);
      }
      return true;
    }
    if (action.GetID() == ACTION_COPY_ITEM)
    {
      if (CanCopy(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnCopy(list);
        if (bDeselect) m_vecItems[list]->Get(item)->Select(false);
      }
      return true;
    }
    if (action.GetID() == ACTION_MOVE_ITEM)
    {
      if (CanMove(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnMove(list);
        if (bDeselect) m_vecItems[list]->Get(item)->Select(false);
      }
      return true;
    }
    if (action.GetID() == ACTION_RENAME_ITEM)
    {
      if (CanRename(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnRename(list);
        if (bDeselect) m_vecItems[list]->Get(item)->Select(false);
      }
      return true;
    }
    if (action.GetID() == ACTION_PARENT_DIR)
    {
      GoParentFolder(list);
      return true;
    }
    if (action.GetID() == ACTION_PLAYER_PLAY)
    {
#ifdef HAS_DVD_DRIVE
      if (m_vecItems[list]->Get(GetSelectedItem(list))->IsDVD())
        return MEDIA_DETECT::CAutorun::PlayDiscAskResume(m_vecItems[list]->Get(GetSelectedItem(list))->GetPath());
#endif
    }
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowFileManager::OnBack(int actionID)
{
  int list = GetFocusedList();
  if (list >= 0 && list <= 1 && actionID == ACTION_NAV_BACK && !m_vecItems[list]->IsVirtualDirectoryRoot())
  {
    GoParentFolder(list);
    return true;
  }
  return CGUIWindow::OnBack(actionID);
}

bool CGUIWindowFileManager::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if window is inactive
      if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
      {
        m_Directory[0]->SetPath("?");
        m_Directory[1]->SetPath("?");
        m_Directory[0]->m_bIsFolder = true;
        m_Directory[1]->m_bIsFolder = true;
        return true;
      }

      //  handle removable media
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        for (int i = 0; i < 2; i++)
        {
          if (m_Directory[i]->IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = GetSelectedItem(i);
            Update(i, m_Directory[i]->GetPath());
            CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem);
          }
          else if (m_Directory[i]->IsRemovable() && !m_rootDir.IsInSource(m_Directory[i]->GetPath()))
          { //
            if (IsActive())
              Update(i, "");
            else
              m_Directory[i]->SetPath("");
          }
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_SOURCES)
      { // State of the sources changed, so update our view
        for (int i = 0; i < 2; i++)
        {
          if (m_Directory[i]->IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = GetSelectedItem(i);
            Update(i, m_Directory[i]->GetPath());
            CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem);
          }
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE && IsActive())
      {
        Refresh();
        return true;
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLIST_CHANGED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    { // send a notify all to all controls on this window
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
      OnMessage(msg);
      break;
    }
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      ClearFileItems(0);
      ClearFileItems(1);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      SetInitialPath(message.GetStringParam());
      message.SetStringParam("");

      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_LEFT_LIST || iControl == CONTROL_RIGHT_LIST)  // list/thumb control
      {
        // get selected item
        int list = iControl - CONTROL_LEFT_LIST;
        int iItem = GetSelectedItem(list);
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_HIGHLIGHT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnMark(list, iItem);
          if (!CInputManager::Get().IsMouseActive())
          {
            //move to next item
            CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), iControl, iItem + 1);
            g_windowManager.SendMessage(msg);
          }
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_DOUBLE_CLICK)
        {
          OnClick(list, iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(list, iItem);
        }
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowFileManager::OnSort(int iList)
{
  // always sort the list by label in ascending order
  for (int i = 0; i < m_vecItems[iList]->Size(); i++)
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(i);
    if (pItem->m_bIsFolder && (!pItem->m_dwSize || pItem->IsPath("add")))
      pItem->SetLabel2("");
    else
      pItem->SetFileSizeLabel();

    // Set free space on disc
    if (pItem->m_bIsShareOrDrive)
    {
      if (pItem->IsHD())
      {
        ULARGE_INTEGER ulBytesFree;
        if (GetDiskFreeSpaceEx(pItem->GetPath().c_str(), &ulBytesFree, NULL, NULL))
        {
          pItem->m_dwSize = ulBytesFree.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
      else if (pItem->IsDVD() && g_mediaManager.IsDiscInDrive())
      {
        ULARGE_INTEGER ulBytesTotal;
        if (GetDiskFreeSpaceEx(pItem->GetPath().c_str(), NULL, &ulBytesTotal, NULL))
        {
          pItem->m_dwSize = ulBytesTotal.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
    } // if (pItem->m_bIsShareOrDrive)

  }

  m_vecItems[iList]->Sort(SortByLabel, SortOrderAscending);
}

void CGUIWindowFileManager::ClearFileItems(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), iList + CONTROL_LEFT_LIST);
  g_windowManager.SendMessage(msg);

  m_vecItems[iList]->Clear(); // will clean up everything
}

void CGUIWindowFileManager::UpdateButtons()
{
  // update our current directory labels
  std::string strDir = CURL(m_Directory[0]->GetPath()).GetWithoutUserDetails();
  if (strDir.empty())
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_LEFT,g_localizeStrings.Get(20108));
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_LEFT, strDir);
  }
  strDir = CURL(m_Directory[1]->GetPath()).GetWithoutUserDetails();
  if (strDir.empty())
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_RIGHT,g_localizeStrings.Get(20108));
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_RIGHT, strDir);
  }

  // update the number of items in each list
  UpdateItemCounts();
}

void CGUIWindowFileManager::UpdateItemCounts()
{
  for (int i = 0; i < 2; i++)
  {
    unsigned int selectedCount = 0;
    unsigned int totalCount = 0;
    int64_t selectedSize = 0;
    int64_t totalSize = 0;
    for (int j = 0; j < m_vecItems[i]->Size(); j++)
    {
      CFileItemPtr item = m_vecItems[i]->Get(j);
      if (item->IsParentFolder()) continue;
      if (item->IsSelected())
      {
        selectedCount++;
        selectedSize += item->m_dwSize;
      }
      totalCount++;
      totalSize += item->m_dwSize;
    }
    std::string items;
    if (selectedCount > 0)
      items = StringUtils::Format("%i/%i %s (%s)", selectedCount, totalCount, g_localizeStrings.Get(127).c_str(), StringUtils::SizeToString(selectedSize).c_str());
    else
      items = StringUtils::Format("%i %s", totalCount, g_localizeStrings.Get(127).c_str());
    SET_CONTROL_LABEL(CONTROL_NUMFILES_LEFT + i, items);
  }
}

bool CGUIWindowFileManager::Update(int iList, const std::string &strDirectory)
{
  // get selected item
  int iItem = GetSelectedItem(iList);
  std::string strSelectedItem = "";

  if (iItem >= 0 && iItem < (int)m_vecItems[iList]->Size())
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(iItem);
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem.get(), strSelectedItem);
      m_history[iList].SetSelectedItem(strSelectedItem, m_Directory[iList]->GetPath());
    }
  }

  std::string strOldDirectory=m_Directory[iList]->GetPath();
  m_Directory[iList]->SetPath(strDirectory);

  CFileItemList items;
  if (!GetDirectory(iList, m_Directory[iList]->GetPath(), items))
  {
    if (strDirectory != strOldDirectory && GetDirectory(iList, strOldDirectory, items))
      m_Directory[iList]->SetPath(strOldDirectory); // Fallback to old (previous) path)
    else
      Update(iList, ""); // Fallback to root

    return false;
  }

  m_history[iList].SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems(iList);

  m_vecItems[iList]->Append(items);
  m_vecItems[iList]->SetPath(items.GetPath());

  std::string strParentPath;
  URIUtils::GetParentPath(strDirectory, strParentPath);
  if (strDirectory.empty() && (m_vecItems[iList]->Size() == 0 || CSettings::Get().GetBool("filelists.showaddsourcebuttons")))
  { // add 'add source button'
    std::string strLabel = g_localizeStrings.Get(1026);
    CFileItemPtr pItem(new CFileItem(strLabel));
    pItem->SetPath("add");
    pItem->SetIconImage("DefaultAddSource.png");
    pItem->SetLabel(strLabel);
    pItem->SetLabelPreformated(true);
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(SortSpecialOnBottom);
    m_vecItems[iList]->Add(pItem);
  }
  else if (items.IsEmpty() || CSettings::Get().GetBool("filelists.showparentdiritems"))
  {
    CFileItemPtr pItem(new CFileItem(".."));
    pItem->SetPath(m_rootDir.IsSource(strDirectory) ? "" : strParentPath);
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    m_vecItems[iList]->AddFront(pItem, 0);
  }

  m_strParentPath[iList] = (m_rootDir.IsSource(strDirectory) ? "" : strParentPath);

  if (strDirectory.empty())
  {
    CFileItemPtr pItem(new CFileItem("special://profile/", true));
    pItem->SetLabel(g_localizeStrings.Get(20070));
    pItem->SetArt("thumb", "DefaultFolder.png");
    pItem->SetLabelPreformated(true);
    m_vecItems[iList]->Add(pItem);
  }

  // if we have a .tbn file, use itself as the thumb
  for (int i = 0; i < (int)m_vecItems[iList]->Size(); i++)
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(i);
    if (pItem->IsHD() &&
        URIUtils::HasExtension(pItem->GetPath(), ".tbn"))
    {
      pItem->SetArt("thumb", pItem->GetPath());
    }
  }
  m_vecItems[iList]->FillInDefaultIcons();

  OnSort(iList);
  UpdateButtons();

  int item = 0;
  strSelectedItem = m_history[iList].GetSelectedItem(m_Directory[iList]->GetPath());
  for (int i = 0; i < m_vecItems[iList]->Size(); ++i)
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(i);
    std::string strHistory;
    GetDirectoryHistoryString(pItem.get(), strHistory);
    if (strHistory == strSelectedItem)
    {
      item = i;
      break;
    }
  }
  UpdateControl(iList, item);
  return true;
}


void CGUIWindowFileManager::OnClick(int iList, int iItem)
{
  if ( iList < 0 || iList >= 2) return ;
  if ( iItem < 0 || iItem >= m_vecItems[iList]->Size() ) return ;

  CFileItemPtr pItem = m_vecItems[iList]->Get(iItem);
  if (pItem->GetPath() == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("files"))
    {
      m_rootDir.SetSources(*CMediaSourceSettings::Get().GetSources("files"));
      Update(0,m_Directory[0]->GetPath());
      Update(1,m_Directory[1]->GetPath());
    }
    return;
  }

  if (!pItem->m_bIsFolder && pItem->IsFileFolder(EFILEFOLDER_MASK_ALL))
  {
    XFILE::IFileDirectory *pFileDirectory = NULL;
    pFileDirectory = XFILE::CFileDirectoryFactory::Create(pItem->GetURL(), pItem.get(), "");
    if(pFileDirectory)
      pItem->m_bIsFolder = true;
    else if(pItem->m_bIsFolder)
      pItem->m_bIsFolder = false;
    delete pFileDirectory;
  }

  if (pItem->m_bIsFolder)
  {
    // save path + drive type because of the possible refresh
    std::string strPath = pItem->GetPath();
    int iDriveType = pItem->m_iDriveType;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem.get(), "files" ) )
      {
        Refresh();
        return ;
      }

      if ( !HaveDiscOrConnection( strPath, iDriveType ) )
        return ;
    }
    if (!Update(iList, strPath))
      ShowShareErrorMessage(pItem.get());
  }
  else if (pItem->IsZIP() || pItem->IsCBZ()) // mount zip archive
  {
    CURL pathToUrl = URIUtils::CreateArchivePath("zip", pItem->GetURL(), "");
    Update(iList, pathToUrl.Get());
  }
  else if (pItem->IsRAR() || pItem->IsCBR())
  {
    CURL pathToUrl = URIUtils::CreateArchivePath("rar", pItem->GetURL(), "");
    Update(iList, pathToUrl.Get());
  }
  else
  {
    OnStart(pItem.get());
    return ;
  }
  // UpdateButtons();
}

// TODO 2.0: Can this be removed, or should we run without the "special" file directories while
// in filemanager view.
void CGUIWindowFileManager::OnStart(CFileItem *pItem)
{
  // start playlists from file manager
  if (pItem->IsPlayList())
  {
    std::string strPlayList = pItem->GetPath();
    unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
    if (NULL != pPlayList.get())
    {
      if (!pPlayList->Load(strPlayList))
      {
        CGUIDialogOK::ShowAndGetInput(6, 477);
        return;
      }
    }
    g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC);
    return;
  }
  if (pItem->IsAudio() || pItem->IsVideo())
  {
    g_application.PlayFile(*pItem);
    return ;
  }
#ifdef HAS_PYTHON
  if (pItem->IsPythonScript())
  {
    CScriptInvocationManager::Get().ExecuteAsync(pItem->GetPath());
    return ;
  }
#endif
  if (pItem->IsPicture())
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return ;
    if (g_application.m_pPlayer->IsPlayingVideo())
      g_application.StopPlaying();

    pSlideShow->Reset();
    pSlideShow->Add(pItem);
    pSlideShow->Select(pItem->GetPath());

    g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}

bool CGUIWindowFileManager::HaveDiscOrConnection( std::string& strPath, int iDriveType )
{
  if ( iDriveType == CMediaSource::SOURCE_TYPE_DVD )
  {
    if ( !g_mediaManager.IsDiscInDrive(strPath) )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219);
      int iList = GetFocusedList();
      int iItem = GetSelectedItem(iList);
      Update(iList, "");
      CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, iItem);
      return false;
    }
  }
  else if ( iDriveType == CMediaSource::SOURCE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !g_application.getNetwork().IsConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221);
      return false;
    }
  }
  else
    return true;
  return true;
}

void CGUIWindowFileManager::UpdateControl(int iList, int item)
{
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), iList + CONTROL_LEFT_LIST, item, 0, m_vecItems[iList]);
  g_windowManager.SendMessage(msg);
}

void CGUIWindowFileManager::OnMark(int iList, int iItem)
{
  CFileItemPtr pItem = m_vecItems[iList]->Get(iItem);

  if (!pItem->m_bIsShareOrDrive)
  {
    if (!pItem->IsParentFolder())
    {
      // MARK file
      pItem->Select(!pItem->IsSelected());
    }
  }

  UpdateItemCounts();
  // UpdateButtons();
}

void CGUIWindowFileManager::OnCopy(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(120, 123))
    return;

  AddJob(new CFileOperationJob(CFileOperationJob::ActionCopy, 
                                *m_vecItems[iList],
                                m_Directory[1 - iList]->GetPath(),
                                true, 16201, 16202));
}

void CGUIWindowFileManager::OnMove(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(121, 124))
    return;

  AddJob(new CFileOperationJob(CFileOperationJob::ActionMove,
                               *m_vecItems[iList],
                               m_Directory[1 - iList]->GetPath(),
                               true, 16203, 16204));
}

void CGUIWindowFileManager::OnDelete(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(122, 125))
    return;

  AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete,
                               *m_vecItems[iList],
                               m_Directory[iList]->GetPath(),
                               true, 16205, 16206));
}

void CGUIWindowFileManager::OnRename(int iList)
{
  std::string strFile;
  for (int i = 0; i < m_vecItems[iList]->Size();++i)
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(i);
    if (pItem->IsSelected())
    {
      strFile = pItem->GetPath();
      break;
    }
  }

  CFileUtils::RenameFile(strFile);

  Refresh(iList);
}

void CGUIWindowFileManager::OnSelectAll(int iList)
{
  for (int i = 0; i < m_vecItems[iList]->Size();++i)
  {
    CFileItemPtr pItem = m_vecItems[iList]->Get(i);
    if (!pItem->IsParentFolder())
    {
      pItem->Select(true);
    }
  }
}

void CGUIWindowFileManager::OnNewFolder(int iList)
{
  std::string strNewFolder = "";
  if (CGUIKeyboardFactory::ShowAndGetInput(strNewFolder, g_localizeStrings.Get(16014), false))
  {
    std::string strNewPath = m_Directory[iList]->GetPath();
    URIUtils::AddSlashAtEnd(strNewPath);
    strNewPath += strNewFolder;
    CDirectory::Create(strNewPath);
    Refresh(iList);

    //  select the new folder
    for (int i=0; i<m_vecItems[iList]->Size(); ++i)
    {
      CFileItemPtr pItem=m_vecItems[iList]->Get(i);
      std::string strPath=pItem->GetPath();
      URIUtils::RemoveSlashAtEnd(strPath);
      if (strPath==strNewPath)
      {
        CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, i);
        break;
      }
    }
  }
}

void CGUIWindowFileManager::Refresh(int iList)
{
  int nSel = GetSelectedItem(iList);
  // update the list views
  Update(iList, m_Directory[iList]->GetPath());

  while (nSel > m_vecItems[iList]->Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}


void CGUIWindowFileManager::Refresh()
{
  int iList = GetFocusedList();
  int nSel = GetSelectedItem(iList);
  // update the list views
  Update(0, m_Directory[0]->GetPath());
  Update(1, m_Directory[1]->GetPath());

  while (nSel > (int)m_vecItems[iList]->Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}

int CGUIWindowFileManager::GetSelectedItem(int iControl)
{
  if (iControl < 0 || iControl > 1 || m_vecItems[iControl]->IsEmpty())
    return -1;
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl + CONTROL_LEFT_LIST);
  if (OnMessage(msg))
    return (int)msg.GetParam1();
  return -1;
}

void CGUIWindowFileManager::GoParentFolder(int iList)
{
  CURL url(m_Directory[iList]->GetPath());
  if (url.IsProtocol("rar") || url.IsProtocol("zip"))
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().empty())
      if (url.IsProtocol("zip"))
        g_ZipManager.release(m_Directory[iList]->GetPath()); // release resources
  }

  std::string strPath(m_strParentPath[iList]), strOldPath(m_Directory[iList]->GetPath());
  Update(iList, strPath);
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowFileManager::GetDirectoryHistoryString(const CFileItem* pItem, std::string& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virtual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      std::string strLabel = pItem->GetLabel();
      size_t nPosOpen = strLabel.find('(');
      size_t nPosClose = strLabel.rfind(')');
      if (nPosOpen != std::string::npos &&
          nPosClose != std::string::npos &&
          nPosClose > nPosOpen)
      {
        strLabel.erase(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virtual directory
      strHistoryString = pItem->GetLabel() + pItem->GetPath();
      URIUtils::RemoveSlashAtEnd(strHistoryString);
    }
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->GetPath();
    URIUtils::RemoveSlashAtEnd(strHistoryString);
  }
}

bool CGUIWindowFileManager::GetDirectory(int iList, const std::string &strDirectory, CFileItemList &items)
{
  const CURL pathToUrl(strDirectory);
  return m_rootDir.GetDirectory(pathToUrl, items, false);
}

bool CGUIWindowFileManager::CanRename(int iList)
{
  // TODO: Renaming of shares (requires writing to sources.xml)
  // this might be able to be done via the webserver code stuff...
  if (m_Directory[iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList]->IsReadOnly()) return false;

  return true;
}

bool CGUIWindowFileManager::CanCopy(int iList)
{
  // can't copy if the destination is not writeable, or if the source is a share!
  // TODO: Perhaps if the source is removeable media (DVD/CD etc.) we could
  // put ripping/backup in here.
  if (!CUtil::SupportsReadFileOperations(m_Directory[iList]->GetPath())) return false;
  if (m_Directory[iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[1 - iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[1 -iList]->IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanMove(int iList)
{
  // can't move if the destination is not writeable, or if the source is a share or not writeable!
  if (m_Directory[0]->IsVirtualDirectoryRoot() || m_Directory[0]->IsReadOnly()) return false;
  if (m_Directory[1]->IsVirtualDirectoryRoot() || m_Directory[1]->IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanDelete(int iList)
{
  if (m_Directory[iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList]->IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanNewFolder(int iList)
{
  if (m_Directory[iList]->IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList]->IsReadOnly()) return false;
  return true;
}

int CGUIWindowFileManager::NumSelected(int iList)
{
  int iSelectedItems = 0;
  for (int iItem = 0; iItem < m_vecItems[iList]->Size(); ++iItem)
  {
    if (m_vecItems[iList]->Get(iItem)->IsSelected()) iSelectedItems++;
  }
  return iSelectedItems;
}

int CGUIWindowFileManager::GetFocusedList() const
{
  return GetFocusedControlID() - CONTROL_LEFT_LIST;
}

void CGUIWindowFileManager::OnPopupMenu(int list, int item, bool bContextDriven /* = true */)
{
  if (list < 0 || list >= 2) return ;
  bool bDeselect = SelectItem(list, item);
  // calculate the position for our menu
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LEFT_LIST + list);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  CFileItemPtr pItem = m_vecItems[list]->Get(item);
  if (!pItem.get())
    return;

  if (m_Directory[list]->IsVirtualDirectoryRoot())
  {
    if (item < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }

    // and do the popup menu
    if (CGUIDialogContextMenu::SourcesMenu("files", pItem, posX, posY))
    {
      m_rootDir.SetSources(*CMediaSourceSettings::Get().GetSources("files"));
      if (m_Directory[1 - list]->IsVirtualDirectoryRoot())
        Refresh();
      else
        Refresh(list);
      return ;
    }
    pItem->Select(false);
    return ;
  }
  // popup the context menu

  bool showEntry = false;
  if (item >= m_vecItems[list]->Size()) item = -1;
  if (item >= 0)
    showEntry=(!pItem->IsParentFolder() || (pItem->IsParentFolder() && m_vecItems[list]->GetSelectedCount()>0));

  // determine available players
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::Get().GetPlayers(*pItem, vecCores);

  // add the needed buttons
  CContextButtons choices;
  if (item >= 0)
  {
    //The ".." item is not selectable. Take that into account when figuring out if all items are selected
    int notSelectable = CSettings::Get().GetBool("filelists.showparentdiritems") ? 1 : 0;
    if (NumSelected(list) <  m_vecItems[list]->Size() - notSelectable)
      choices.Add(1, 188); // SelectAll
    if (!pItem->IsParentFolder())
      choices.Add(2,  XFILE::CFavouritesDirectory::IsFavourite(pItem.get(), GetID()) ? 14077 : 14076); // Add/Remove Favourite
    if (vecCores.size() > 1)
      choices.Add(3, 15213); // Play Using...
    if (CanRename(list) && !pItem->IsParentFolder())
      choices.Add(4, 118); // Rename
    if (CanDelete(list) && showEntry)
      choices.Add(5, 117); // Delete
    if (CanCopy(list) && showEntry)
      choices.Add(6, 115); // Copy
    if (CanMove(list) && showEntry)
      choices.Add(7, 116); // Move
  }
  if (CanNewFolder(list))
    choices.Add(8, 20309); // New Folder
  if (item >= 0 && pItem->m_bIsFolder && !pItem->IsParentFolder())
    choices.Add(9, 13393); // Calculate Size
  choices.Add(11, 20128); // Go To Root
  choices.Add(12, 523);     // switch media
  if (CJobManager::GetInstance().IsProcessing("filemanager"))
    choices.Add(13, 167);

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (btnid == 1)
  {
    OnSelectAll(list);
    bDeselect=false;
  }
  if (btnid == 2)
  {
    XFILE::CFavouritesDirectory::AddOrRemove(pItem.get(), GetID());
    return;
  }
  if (btnid == 3)
  {
    VECPLAYERCORES vecCores;
    CPlayerCoreFactory::Get().GetPlayers(*pItem, vecCores);
    g_application.m_eForcedNextPlayer = CPlayerCoreFactory::Get().SelectPlayerDialog(vecCores);
    if (g_application.m_eForcedNextPlayer != EPC_NONE)
      OnStart(pItem.get());
  }
  if (btnid == 4)
    OnRename(list);
  if (btnid == 5)
    OnDelete(list);
  if (btnid == 6)
    OnCopy(list);
  if (btnid == 7)
    OnMove(list);
  if (btnid == 8)
    OnNewFolder(list);
  if (btnid == 9)
  {
    // setup the progress dialog, and show it
    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(13394);
      for (int i=0; i < 3; i++)
        progress->SetLine(i, "");
      progress->StartModal();
    }

    //  Calculate folder size for each selected item
    for (int i=0; i<m_vecItems[list]->Size(); ++i)
    {
      CFileItemPtr pItem2=m_vecItems[list]->Get(i);
      if (pItem2->m_bIsFolder && pItem2->IsSelected())
      {
        int64_t folderSize = CalculateFolderSize(pItem2->GetPath(), progress);
        if (folderSize >= 0)
        {
          pItem2->m_dwSize = folderSize;
          if (folderSize == 0)
            pItem2->SetLabel2(StringUtils::SizeToString(folderSize));
          else
            pItem2->SetFileSizeLabel();
        }
      }
    }
    if (progress)
      progress->Close();
  }
  if (btnid == 11)
  {
    Update(list,"");
    return;
  }
  if (btnid == 12)
  {
    CGUIDialogContextMenu::SwitchMedia("files", m_vecItems[list]->GetPath());
    return;
  }
  if (btnid == 13)
    CancelJobs();

  if (bDeselect && item >= 0 && item < m_vecItems[list]->Size())
  { // deselect item as we didn't do anything
    pItem->Select(false);
  }
}

// Highlights the item in the list under the cursor
// returns true if we should deselect the item, false otherwise
bool CGUIWindowFileManager::SelectItem(int list, int &item)
{
  // get the currently selected item in the list
  item = GetSelectedItem(list);

  // select the item if we need to
  if (item > -1 && !NumSelected(list) && !m_vecItems[list]->Get(item)->IsParentFolder())
  {
    m_vecItems[list]->Get(item)->Select(true);
    return true;
  }
  return false;
}

// recursively calculates the selected folder size
int64_t CGUIWindowFileManager::CalculateFolderSize(const std::string &strDirectory, CGUIDialogProgress *pProgress)
{
  const CURL pathToUrl(strDirectory);
  if (pProgress)
  { // update our progress control
    pProgress->Progress();
    pProgress->SetLine(1, strDirectory);
    if (pProgress->IsCanceled())
      return -1;
  }
  // start by calculating the size of the files in this folder...
  int64_t totalSize = 0;
  CFileItemList items;
  CVirtualDirectory rootDir;
  rootDir.SetSources(*CMediaSourceSettings::Get().GetSources("files"));
  rootDir.GetDirectory(pathToUrl, items, false);
  for (int i=0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder && !items[i]->IsParentFolder()) // folder
    {
      int64_t folderSize = CalculateFolderSize(items[i]->GetPath(), pProgress);
      if (folderSize < 0) return -1;
      totalSize += folderSize;
    }
    else // file
      totalSize += items[i]->m_dwSize;
  }
  return totalSize;
}

void CGUIWindowFileManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if(!success)
  {
    CFileOperationJob* fileJob = (CFileOperationJob*)job;
    CGUIDialogOK::ShowAndGetInput(fileJob->GetHeading(),
                                  fileJob->GetLine(), 16200, 0);
  }

  if (IsActive())
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_UPDATE);
    CApplicationMessenger::Get().SendGUIMessage(msg, GetID(), false);
  }

  CJobQueue::OnJobComplete(jobID, success, job);
}

void CGUIWindowFileManager::ShowShareErrorMessage(CFileItem* pItem)
{
  int idMessageText = 0;
  CURL url(pItem->GetPath());

  if (url.IsProtocol("smb") && url.GetHostName().empty()) //  smb workgroup
    idMessageText = 15303; // Workgroup not found
  else if (pItem->m_iDriveType == CMediaSource::SOURCE_TYPE_REMOTE || URIUtils::IsRemote(pItem->GetPath()))
    idMessageText = 15301; // Could not connect to network server
  else
    idMessageText = 15300; // Path not found or invalid

  CGUIDialogOK::ShowAndGetInput(220, idMessageText);
}

void CGUIWindowFileManager::OnInitWindow()
{
  bool bResult0 = Update(0, m_Directory[0]->GetPath());
  bool bResult1 = Update(1, m_Directory[1]->GetPath());

  CGUIWindow::OnInitWindow();

  if (!bCheckShareConnectivity)
  {
    bCheckShareConnectivity = true; //reset
    CFileItem pItem(strCheckSharePath, true);
    ShowShareErrorMessage(&pItem); //show the error message after window is loaded!
    Update(0,""); // reset view to root
  }
  else if (!bResult0)
  {
    ShowShareErrorMessage(m_Directory[0]); //show the error message after window is loaded!
  }

  if (!bResult1)
  {
    ShowShareErrorMessage(m_Directory[1]); //show the error message after window is loaded!
  }
}

void CGUIWindowFileManager::SetInitialPath(const std::string &path)
{
  // check for a passed destination path
  std::string strDestination = path;
  m_rootDir.SetSources(*CMediaSourceSettings::Get().GetSources("files"));
  if (!strDestination.empty())
  {
    CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
  }
  // otherwise, is this the first time accessing this window?
  else if (m_Directory[0]->GetPath() == "?")
  {
    m_Directory[0]->SetPath(strDestination = CMediaSourceSettings::Get().GetDefaultSource("files"));
    CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
  }
  // try to open the destination path
  if (!strDestination.empty())
  {
    // open root
    if (StringUtils::EqualsNoCase(strDestination, "$ROOT"))
    {
      m_Directory[0]->SetPath("");
      CLog::Log(LOGINFO, "  Success! Opening root listing.");
    }
    else
    {
      // default parameters if the jump fails
      m_Directory[0]->SetPath("");

      bool bIsSourceName = false;
      VECSOURCES shares;
      m_rootDir.GetSources(shares);
      int iIndex = CUtil::GetMatchingSource(strDestination, shares, bIsSourceName);
      if (iIndex > -1)
      {
        // set current directory to matching share
        std::string path;
        if (bIsSourceName && iIndex < (int)shares.size())
          path = shares[iIndex].strPath;
        else
          path = strDestination;
        URIUtils::RemoveSlashAtEnd(path);
        m_Directory[0]->SetPath(path);
        CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());

        // outside call: check the share for connectivity
        bCheckShareConnectivity = Update(0, m_Directory[0]->GetPath());
        if(!bCheckShareConnectivity)
          strCheckSharePath = m_Directory[0]->GetPath();
      }
      else
      {
        CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
      }
    }
  }

  if (m_Directory[1]->GetPath() == "?") m_Directory[1]->SetPath("");
}

const CFileItem& CGUIWindowFileManager::CurrentDirectory(int indx) const
{
  return *m_Directory[indx];
}
