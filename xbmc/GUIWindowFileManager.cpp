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
#include "GUIWindowFileManager.h"
#include "Application.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "FileSystem/Directory.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/FactoryFileDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "Picture.h"
#include "GUIDialogContextMenu.h"
#include "GUIListContainer.h"
#include "GUIDialogMediaSource.h"
#include "GUIPassword.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "GUIWindowSlideShow.h"
#include "PlayListFactory.h"
#include "utils/GUIInfoManager.h"
#include "xbox/Network.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;

#define ACTION_COPY   1
#define ACTION_MOVE   2
#define ACTION_DELETE 3
#define ACTION_CREATEFOLDER 4
#define ACTION_DELETEFOLDER 5

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4
#define CONTROL_BTNNEWFOLDER      6
#define CONTROL_BTNSELECTALL   7
#define CONTROL_BTNCOPY           10
#define CONTROL_BTNMOVE           11
#define CONTROL_BTNDELETE         8
#define CONTROL_BTNRENAME         9

#define CONTROL_NUMFILES_LEFT   12
#define CONTROL_NUMFILES_RIGHT  13

#define CONTROL_LEFT_LIST     20
#define CONTROL_RIGHT_LIST    21

#define CONTROL_CURRENTDIRLABEL_LEFT  101
#define CONTROL_CURRENTDIRLABEL_RIGHT  102

CGUIWindowFileManager::CGUIWindowFileManager(void)
    : CGUIWindow(WINDOW_FILES, "FileManager.xml")
{
  m_dlgProgress = NULL;
  m_Directory[0].m_strPath = "?";
  m_Directory[1].m_strPath = "?";
  m_Directory[0].m_bIsFolder = true;
  m_Directory[1].m_bIsFolder = true;
  bCheckShareConnectivity = true;
}

CGUIWindowFileManager::~CGUIWindowFileManager(void)
{}

bool CGUIWindowFileManager::OnAction(const CAction &action)
{
  int item;
  int list = GetFocusedList();
  if (list >= 0 && list <= 1)
  {
    // the non-contextual menu can be called at any time
    if (action.wID == ACTION_CONTEXT_MENU && m_vecItems[list].Size() == 0)
    {
      OnPopupMenu(list,-1, false);
      return true;
    }
    if (action.wID == ACTION_DELETE_ITEM)
    {
      if (CanDelete(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnDelete(list);
        if (bDeselect) m_vecItems[list][item]->Select(false);
      }
      return true;
    }
    if (action.wID == ACTION_COPY_ITEM)
    {
      if (CanCopy(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnCopy(list);
        if (bDeselect) m_vecItems[list][item]->Select(false);
      }
      return true;
    }
    if (action.wID == ACTION_MOVE_ITEM)
    {
      if (CanMove(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnMove(list);
        if (bDeselect) m_vecItems[list][item]->Select(false);
      }
      return true;
    }
    if (action.wID == ACTION_RENAME_ITEM)
    {
      if (CanRename(list))
      {
        bool bDeselect = SelectItem(list, item);
        OnRename(list);
        if (bDeselect) m_vecItems[list][item]->Select(false);
      }
      return true;
    }
    if (action.wID == ACTION_PARENT_DIR)
    {
      if (m_vecItems[list].IsVirtualDirectoryRoot())
        m_gWindowManager.PreviousWindow();
      else
        GoParentFolder(list);
      return true;
    }
    if (action.wID == ACTION_PLAYER_PLAY)
    {
      if (m_vecItems[list][GetSelectedItem(list)]->IsDVD())
        return CAutorun::PlayDisc();
    }
  }
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowFileManager::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if window is inactive
      if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
      {
        m_Directory[0].m_strPath = "?";
        m_Directory[1].m_strPath = "?";
        m_Directory[0].m_bIsFolder = true;
        m_Directory[1].m_bIsFolder = true;
        return true;
      }

      //  handle removable media
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        for (int i = 0; i < 2; i++)
        {
          if (m_Directory[i].IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = GetSelectedItem(i);
            Update(i, m_Directory[i].m_strPath);
            CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem)
          }
          else if (m_Directory[i].IsRemovable() && !m_rootDir.IsInShare(m_Directory[i].m_strPath))
          { //
            if (IsActive())
              Update(i, "");
            else
              m_Directory[i].m_strPath="";
          }
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_SOURCES)
      { // State of the sources changed, so update our view
        for (int i = 0; i < 2; i++)
        {
          if (m_Directory[i].IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = GetSelectedItem(i);
            Update(i, m_Directory[i].m_strPath);
            CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem)
          }
        }
        return true;
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      m_dlgProgress = NULL;
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
          if (!g_Mouse.IsActive())
          {
            //move to next item
            CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), iControl, iItem + 1, 0, NULL);
            g_graphicsContext.SendMessage(msg);
          }
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_DOUBLE_CLICK)
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
  for (int i = 0; i < m_vecItems[iList].Size(); i++)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (pItem->m_bIsFolder && !pItem->m_dwSize)
      pItem->SetLabel2("");
    else
      pItem->SetFileSizeLabel();

    // Set free space on disc
    if (pItem->m_bIsShareOrDrive)
    {
      if (pItem->IsHD())
      {
        ULARGE_INTEGER ulBytesFree;
        if (GetDiskFreeSpaceEx(pItem->m_strPath.c_str(), &ulBytesFree, NULL, NULL))
        {
          pItem->m_dwSize = ulBytesFree.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
      else if (pItem->IsDVD() && CDetectDVDMedia::IsDiscInDrive())
      {
        ULARGE_INTEGER ulBytesTotal;
        if (GetDiskFreeSpaceEx(pItem->m_strPath.c_str(), NULL, &ulBytesTotal, NULL))
        {
          pItem->m_dwSize = ulBytesTotal.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
    } // if (pItem->m_bIsShareOrDrive)

  }

  m_vecItems[iList].Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  // UpdateControl(iList);
}

void CGUIWindowFileManager::ClearFileItems(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  m_vecItems[iList].Clear(); // will clean up everything
}

void CGUIWindowFileManager::UpdateButtons()
{

  /*
   // Update sorting control
   bool bSortOrder=false;
   if ( m_bViewSource )
   {
    if (m_strSourceDirectory.IsEmpty())
     bSortOrder=g_stSettings.m_bMyFilesSourceRootSortOrder;
    else
     bSortOrder=g_stSettings.m_bMyFilesSourceSortOrder;
   }
   else
   {
    if (m_strDestDirectory.IsEmpty())
     bSortOrder=g_stSettings.m_bMyFilesDestRootSortOrder;
    else
     bSortOrder=g_stSettings.m_bMyFilesDestSortOrder;
   }

   if (bSortOrder)
    {
      CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }
    else
    {
      CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }

  */
  // update our current directory labels
  CStdString strDir;
  CURL(m_Directory[0].m_strPath).GetURLWithoutUserDetails(strDir);
  if (strDir.IsEmpty())
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_LEFT,g_localizeStrings.Get(20108));
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_LEFT, strDir);
  }
  CURL(m_Directory[1].m_strPath).GetURLWithoutUserDetails(strDir);
  if (strDir.IsEmpty())
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
    __int64 selectedSize = 0;
    __int64 totalSize = 0;
    for (int j = 0; j < m_vecItems[i].Size(); j++)
    {
      CFileItem *item = m_vecItems[i][j];
      if (item->IsParentFolder()) continue;
      if (item->IsSelected())
      {
        selectedCount++;
        selectedSize += item->m_dwSize;
      }
      totalCount++;
      totalSize += item->m_dwSize;
    }
    CStdString items;
    if (selectedCount > 0)
      items.Format("%i/%i %s (%s)", selectedCount, totalCount, g_localizeStrings.Get(127).c_str(), StringUtils::SizeToString(selectedSize).c_str());
    else
      items.Format("%i %s", totalCount, g_localizeStrings.Get(127).c_str());
    SET_CONTROL_LABEL(CONTROL_NUMFILES_LEFT + i, items);
  }
}

bool CGUIWindowFileManager::Update(int iList, const CStdString &strDirectory)
{
  // get selected item
  int iItem = GetSelectedItem(iList);
  CStdString strSelectedItem = "";

  if (iItem >= 0 && iItem < (int)m_vecItems[iList].Size())
  {
    CFileItem* pItem = m_vecItems[iList][iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
      m_history[iList].SetSelectedItem(strSelectedItem, m_Directory[iList].m_strPath);
    }
  }

  CStdString strOldDirectory=m_Directory[iList].m_strPath;
  m_Directory[iList].m_strPath = strDirectory;

  CFileItemList items;
  if (!GetDirectory(iList, m_Directory[iList].m_strPath, items))
  {
    m_Directory[iList].m_strPath = strOldDirectory;
    return false;
  }

  m_history[iList].SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems(iList);

  m_vecItems[iList].AppendPointer(items);
  m_vecItems[iList].m_strPath = items.m_strPath;
  items.ClearKeepPointer();

  if (strDirectory.IsEmpty() && (m_vecItems[iList].Size() == 0 || !g_guiSettings.GetBool("filelists.disableaddsourcebuttons")))
  { // add 'add source button'
    CStdString strLabel = g_localizeStrings.Get(1026);
    CFileItem *pItem = new CFileItem(strLabel);
    pItem->m_strPath = "add";
    pItem->SetThumbnailImage("DefaultAddSource.png");
    pItem->SetLabel(strLabel);
    pItem->SetLabelPreformated(true);
    m_vecItems[iList].Add(pItem);
  }

  // if we have a .tbn file, use itself as the thumb
  for (int i = 0; i < (int)m_vecItems[iList].Size(); i++)
  {
    CFileItem *pItem = m_vecItems[iList][i];
    CStdString strExtension;
    CUtil::GetExtension(pItem->m_strPath, strExtension);
    if (pItem->IsHD() && strExtension == ".tbn")
    {
      pItem->SetThumbnailImage(pItem->m_strPath);
    }
  }
  m_vecItems[iList].FillInDefaultIcons();

  OnSort(iList);
  UpdateButtons();
  UpdateControl(iList);

  bool foundItem(false);
  strSelectedItem = m_history[iList].GetSelectedItem(m_Directory[iList].m_strPath);
  for (int i = 0; i < m_vecItems[iList].Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    CStdString strHistory;
    GetDirectoryHistoryString(pItem, strHistory);
    if (strHistory == strSelectedItem)
    {
      CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, i);
      foundItem = true;
      break;
    }
  }
  if (!foundItem)
  { // select the first item
    CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, 0);
  }
  return true;
}


void CGUIWindowFileManager::OnClick(int iList, int iItem)
{
  if ( iList < 0 || iList > 2) return ;
  if ( iItem < 0 || iItem >= m_vecItems[iList].Size() ) return ;

  CFileItem *pItem = m_vecItems[iList][iItem];
  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("files"))
    {
      Update(0,m_Directory[0].m_strPath);
      Update(1,m_Directory[1].m_strPath);
    }
    return;
  }

  if (pItem->m_bIsFolder)
  {
    // save path + drive type because of the possible refresh
    CStdString strPath = pItem->m_strPath;
    int iDriveType = pItem->m_iDriveType;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "files" ) )
      {
        Refresh();
        return ;
      }

      if ( !HaveDiscOrConnection( strPath, iDriveType ) )
        return ;
    }
    if (!Update(iList, strPath))
      ShowShareErrorMessage(pItem);
  }
  else if (pItem->IsZIP() || pItem->IsCBZ()) // mount zip archive
  {
    CStdString strArcivedPath;
    CUtil::CreateZipPath(strArcivedPath, pItem->m_strPath, "");
    Update(iList, strArcivedPath);
  }
  else if (pItem->IsRAR() || pItem->IsCBR())
  {
    CStdString strArcivedPath;
    CUtil::CreateRarPath(strArcivedPath, pItem->m_strPath, "");
    Update(iList, strArcivedPath);
  }
  else
  {
    OnStart(pItem);
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
    CStdString strPlayList = pItem->m_strPath;
    auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
    if (NULL != pPlayList.get())
    {
      if (!pPlayList->Load(strPlayList))
      {
        CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
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
    g_pythonParser.evalFile(pItem->m_strPath.c_str());
    return ;
  }
#endif
  if (pItem->IsXBE())
  {
    int iRegion;
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(pItem->m_strPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      iRegion = xbe.FilterRegion(iRegion);
    }
    else
      iRegion = 0;
    CUtil::RunXBE(pItem->m_strPath.c_str(),NULL,F_VIDEO(iRegion));
  }
  else if (pItem->IsShortCut())
    CUtil::RunShortcut(pItem->m_strPath);
  if (pItem->IsPicture())
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return ;
    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    pSlideShow->Reset();
    pSlideShow->Add(pItem);
    pSlideShow->Select(pItem->m_strPath);

    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}

bool CGUIWindowFileManager::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    CDetectDVDMedia::WaitMediaReady();

    if ( !CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      int iList = GetFocusedList();
      int iItem = GetSelectedItem(iList);
      Update(iList, "");
      CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, iItem)
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

void CGUIWindowFileManager::UpdateControl(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  for (int i = 0; i < m_vecItems[iList].Size(); i++)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, (void*)pItem);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIWindowFileManager::OnMark(int iList, int iItem)
{
  CFileItem* pItem = m_vecItems[iList][iItem];

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

bool CGUIWindowFileManager::DoProcessFile(int iAction, const CStdString& strFile, const CStdString& strDestFile)
{
  CStdString strShortSourceFile;
  CStdString strShortDestFile;
  CURL(strFile).GetURLWithoutUserDetails(strShortSourceFile);
  CURL(strDestFile).GetURLWithoutUserDetails(strShortDestFile);
  switch (iAction)
  {
  case ACTION_COPY:
    {
      CLog::Log(LOGDEBUG,"FileManager: copy %s->%s\n", strFile.c_str(), strDestFile.c_str());

      CURL url(strFile);
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 115);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, strShortDestFile);
        m_dlgProgress->Progress();
      }

      if (url.GetProtocol() == "rar")
      {
        g_RarManager.SetWipeAtWill(false);
        CStdString strOriginalCachePath = g_advancedSettings.m_cachePath;
        CStdString strDestPath;
        CUtil::GetDirectory(strDestFile,strDestPath);
        g_advancedSettings.m_cachePath = strDestPath;
        CLog::Log(LOGDEBUG, "CacheRarredFile: dest=%s, file=%s",strDestPath.c_str(), url.GetFileName().c_str());
        bool bResult = g_RarManager.CacheRarredFile(strDestPath,url.GetHostName(),url.GetFileName(),0,strDestPath,-2);
        g_advancedSettings.m_cachePath = strOriginalCachePath;
        g_RarManager.SetWipeAtWill(true);
        return bResult;
      }
      else
      {
        if (!CFile::Cache(strFile, strDestFile, this, NULL))
          return false;
      }
    }
    break;


  case ACTION_MOVE:
    {
      CLog::Log(LOGDEBUG,"FileManager: move %s->%s\n", strFile.c_str(), strDestFile.c_str());

      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 116);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, strShortDestFile);
        m_dlgProgress->Progress();
      }

#ifndef _LINUX
      if (strFile[1] == ':' && strFile[0] == strDestFile[0])
      {
        // quick move on same drive
        CFile::Rename(strFile, strDestFile);
      }
      else
      {
        if (CFile::Cache(strFile, strDestFile, this, NULL ) )
        {
          CFile::Delete(strFile);
        }
        else
          return false;
      }
#else
      CFile::Rename(strFile, strDestFile);
#endif
    }
    break;

  case ACTION_DELETE:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete %s\n", strFile.c_str());

      CFile::Delete(strFile);
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 117);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;

  case ACTION_DELETEFOLDER:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete folder %s\n", strFile.c_str());

      CDirectory::Remove(strFile);
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 117);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;

  case ACTION_CREATEFOLDER:
    {
      CLog::Log(LOGDEBUG,"FileManager: create folder %s\n", strFile.c_str());

      CDirectory::Create(strFile);

      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 119);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;
  }

  bool bResult = true;
  if (m_dlgProgress)
  {
    m_dlgProgress->Progress();
    bResult = !m_dlgProgress->IsCanceled();
  }
  return bResult;
}

bool CGUIWindowFileManager::DoProcessFolder(int iAction, const CStdString& strPath, const CStdString& strDestFile)
{
  // check whether this folder is a filedirectory - if so, we don't process it's contents
  CFileItem item(strPath, false);
  if (CFactoryFileDirectory::Create(strPath, &item))
    return true;
  CLog::Log(LOGDEBUG,"FileManager, processing folder: %s",strPath.c_str());
  CFileItemList items;
  //m_rootDir.GetDirectory(strPath, items);
  CDirectory::GetDirectory(strPath, items, "", false);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem* pItem = items[i];
    pItem->Select(true);
    CLog::Log(LOGDEBUG,"  -- %s",pItem->m_strPath.c_str());
  }

  if (!DoProcess(iAction, items, strDestFile)) return false;

  if (iAction == ACTION_MOVE)
  {
    CDirectory::Remove(strPath);
  }
  return true;
}

bool CGUIWindowFileManager::DoProcess(int iAction, CFileItemList & items, const CStdString& strDestFile)
{
  bool bCancelled(false);
  for (int iItem = 0; iItem < items.Size(); ++iItem)
  {
    CFileItem* pItem = items[iItem];
    if (pItem->IsSelected())
    {
      CStdString strNoSlash = pItem->m_strPath;
      CUtil::RemoveSlashAtEnd(strNoSlash);
      CStdString strFileName = CUtil::GetFileName(strNoSlash);

      // special case for upnp
      if (CUtil::IsUPnP(items.m_strPath) || CUtil::IsUPnP(pItem->m_strPath))
      {
        // get filename from label instead of path
        strFileName = pItem->GetLabel();

        if(!pItem->m_bIsFolder && CUtil::GetExtension(strFileName).length() == 0)
        {
          // FIXME: for now we only work well if the url has the extension
          // we should map the content type to the extension otherwise
          strFileName += CUtil::GetExtension(pItem->m_strPath);
        }

        CUtil::RemoveIllegalChars(strFileName);
        CUtil::ShortenFileName(strFileName);
      }

      CStdString strnewDestFile;
      if(!strDestFile.IsEmpty()) // only do this if we have a destination
        CUtil::AddFileToFolder(strDestFile, strFileName, strnewDestFile);

      if (pItem->m_bIsFolder)
      {
        // create folder on dest. drive
        if (iAction != ACTION_DELETE)
        {
          CLog::Log(LOGDEBUG, "Create folder with strDestFile=%s, strnewDestFile=%s, pFileName=%s", strDestFile.c_str(), strnewDestFile.c_str(), strFileName.c_str());
          if (!DoProcessFile(ACTION_CREATEFOLDER, strnewDestFile, "")) return false;
        }
        if (!DoProcessFolder(iAction, pItem->m_strPath, strnewDestFile)) return false;
        if (iAction == ACTION_DELETE)
        {
          if (!DoProcessFile(ACTION_DELETEFOLDER, pItem->m_strPath, "")) return false;
        }
      }
      else
      {
        if (!DoProcessFile(iAction, pItem->m_strPath, strnewDestFile)) return false;
      }
    }
  }
  return true;
}

void CGUIWindowFileManager::OnCopy(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(120, 123, 0, 0))
    return;

  ResetProgressBar();

  bool success = DoProcess(ACTION_COPY, m_vecItems[iList], m_Directory[1 - iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  if(!success)
    CGUIDialogOK::ShowAndGetInput(16201, 16202, 16200, 0);

  Refresh(1 - iList);
}

void CGUIWindowFileManager::OnMove(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(121, 124, 0, 0))
    return;

  ResetProgressBar();

  bool success = DoProcess(ACTION_MOVE, m_vecItems[iList], m_Directory[1 - iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  if(!success)
    CGUIDialogOK::ShowAndGetInput(16203, 16204, 16200, 0);

  Refresh();
}

void CGUIWindowFileManager::OnDelete(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(122, 125, 0, 0))
    return;

  ResetProgressBar(false);

  bool success = DoProcess(ACTION_DELETE, m_vecItems[iList], m_Directory[iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  if(!success)
    CGUIDialogOK::ShowAndGetInput(16205, 16206, 16200, 0);

  Refresh(iList);
}

void CGUIWindowFileManager::OnRename(int iList)
{
  CStdString strFile;
  for (int i = 0; i < m_vecItems[iList].Size();++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (pItem->IsSelected())
    {
      strFile = pItem->m_strPath;
      break;
    }
  }

  RenameFile(strFile);

  Refresh(iList);
}

void CGUIWindowFileManager::OnSelectAll(int iList)
{
  for (int i = 0; i < m_vecItems[iList].Size();++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (!pItem->IsParentFolder())
    {
      pItem->Select(true);
    }
  }
}

bool CGUIWindowFileManager::RenameFile(const CStdString &strFile)
{
  CStdString strFileAndPath(strFile);
  CUtil::RemoveSlashAtEnd(strFileAndPath);
  CStdString strFileName = CUtil::GetFileName(strFileAndPath);
  CStdString strPath = strFile.Left(strFileAndPath.size() - strFileName.size());
  if (CGUIDialogKeyboard::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath += strFileName;
    CLog::Log(LOGINFO,"FileManager: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (CUtil::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      vector<CStdString> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        CStdString filePath(paths[i]);
        CUtil::RemoveSlashAtEnd(filePath);
        CUtil::GetDirectory(filePath, filePath);
        CUtil::AddFileToFolder(filePath, strFileName, filePath);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}

void CGUIWindowFileManager::OnNewFolder(int iList)
{
  CStdString strNewFolder = "";
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFolder, g_localizeStrings.Get(16014), false))
  {
    CStdString strNewPath = m_Directory[iList].m_strPath;
    if (!CUtil::HasSlashAtEnd(strNewPath) ) CUtil::AddSlashAtEnd(strNewPath);
    strNewPath += strNewFolder;
    CDirectory::Create(strNewPath);
    Refresh(iList);

    //  select the new folder
    for (int i=0; i<m_vecItems[iList].Size(); ++i)
    {
      CFileItem* pItem=m_vecItems[iList][i];
      CStdString strPath=pItem->m_strPath;
      if (CUtil::HasSlashAtEnd(strPath)) CUtil::RemoveSlashAtEnd(strPath);
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
  Update(iList, m_Directory[iList].m_strPath);

  // UpdateButtons();
  // UpdateControl(iList);

  while (nSel > m_vecItems[iList].Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}


void CGUIWindowFileManager::Refresh()
{
  int iList = GetFocusedList();
  int nSel = GetSelectedItem(iList);
  // update the list views
  Update(0, m_Directory[0].m_strPath);
  Update(1, m_Directory[1].m_strPath);

  // UpdateButtons();
  //  UpdateControl(0);
  // UpdateControl(1);

  while (nSel > (int)m_vecItems[iList].Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}

int CGUIWindowFileManager::GetSelectedItem(int iControl)
{
  if (iControl < 0 || iControl > 1) return -1;
  CGUIListContainer *pControl = (CGUIListContainer *)GetControl(iControl + CONTROL_LEFT_LIST);
  if (!pControl || !m_vecItems[iControl].Size()) return -1;
  return pControl->GetSelectedItem();
}

void CGUIWindowFileManager::GoParentFolder(int iList)
{
  CURL url(m_Directory[iList].m_strPath);
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip"))
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
      if (url.GetProtocol() == "zip")
        g_ZipManager.release(m_Directory[iList].m_strPath); // release resources
  }

  CStdString strPath(m_strParentPath[iList]), strOldPath(m_Directory[iList].m_strPath);
  Update(iList, strPath);

  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
    m_history[iList].RemoveSelectedItem(strOldPath); //Delete current path
}

void CGUIWindowFileManager::Render()
{
  CGUIWindow::Render();
}

bool CGUIWindowFileManager::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetPercentage(ipercent);
    CStdString speedString;
    speedString.Format("%2.2f KB/s", avgSpeed / 1024);
    m_dlgProgress->SetLine(0, speedString);
    m_dlgProgress->Progress();
    if (m_dlgProgress->IsCanceled()) return false;
  }
  return true;
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowFileManager::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virtual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == SHARE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virtual directory
      strHistoryString = pItem->GetLabel() + pItem->m_strPath;
      CUtil::RemoveSlashAtEnd(strHistoryString);
    }
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;
    CUtil::RemoveSlashAtEnd(strHistoryString);
  }
}

bool CGUIWindowFileManager::GetDirectory(int iList, const CStdString &strDirectory, CFileItemList &items)
{
  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("filelists.hideparentdiritems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        items.Add(pItem);
      }
      m_strParentPath[iList] = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("filelists.hideparentdiritems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }
    m_strParentPath[iList] = "";
  }

  bool bResult = m_rootDir.GetDirectory(strDirectory,items,false);
  if (strDirectory.IsEmpty() && items.Size() == 0)
  {
    CStdString strLabel = g_localizeStrings.Get(1026);
    CFileItem *pItem = new CFileItem(strLabel);
    pItem->m_strPath = "add";
    pItem->SetThumbnailImage("settings-network-focus.png");
    pItem->SetLabel(strLabel);
    pItem->SetLabelPreformated(true);
    items.Add(pItem);
  }
  return bResult;
}

bool CGUIWindowFileManager::CanRename(int iList)
{
  // TODO: Renaming of shares (requires writing to xboxmediacenter.xml)
  // this might be able to be done via the webserver code stuff...
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;

  return true;
}

bool CGUIWindowFileManager::CanCopy(int iList)
{
  // can't copy if the destination is not writeable, or if the source is a share!
  // TODO: Perhaps if the source is removeable media (DVD/CD etc.) we could
  // put ripping/backup in here.
  if (m_Directory[1 - iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[1 -iList].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanMove(int iList)
{
  // can't move if the destination is not writeable, or if the source is a share or not writeable!
  if (m_Directory[0].IsVirtualDirectoryRoot() || m_Directory[0].IsReadOnly()) return false;
  if (m_Directory[1].IsVirtualDirectoryRoot() || m_Directory[1].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanDelete(int iList)
{
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanNewFolder(int iList)
{
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;
  return true;
}

int CGUIWindowFileManager::NumSelected(int iList)
{
  int iSelectedItems = 0;
  for (int iItem = 0; iItem < m_vecItems[iList].Size(); ++iItem)
  {
    if (m_vecItems[iList][iItem]->IsSelected()) iSelectedItems++;
  }
  return iSelectedItems;
}

int CGUIWindowFileManager::GetFocusedList() const
{
  return GetFocusedControlID() - CONTROL_LEFT_LIST;
}

void CGUIWindowFileManager::OnPopupMenu(int list, int item, bool bContextDriven /* = true */)
{
  if (list < 0 || list > 2) return ;
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
  if (m_Directory[list].IsVirtualDirectoryRoot())
  {
    if (item < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }

    // and do the popup menu
    if (CGUIDialogContextMenu::SourcesMenu("files", m_vecItems[list][item], posX, posY))
    {
      m_rootDir.SetShares(g_settings.m_vecMyFilesShares);
      if (m_Directory[1 - list].IsVirtualDirectoryRoot())
        Refresh();
      else
        Refresh(list);
      return ;
    }
    m_vecItems[list][item]->Select(false);
    return ;
  }
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    bool showEntry = false;
    if (item >= m_vecItems[list].Size()) item = -1;
    if (item >= 0)
      showEntry=(!m_vecItems[list][item]->IsParentFolder() || (m_vecItems[list][item]->IsParentFolder() && m_vecItems[list].GetSelectedCount()>0));
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    int btn_SelectAll = pMenu->AddButton(188); // SelectAll
    int btn_Rename = pMenu->AddButton(118); // Rename
    int btn_Delete = pMenu->AddButton(117); // Delete
    int btn_Copy = pMenu->AddButton(115); // Copy
    int btn_Move = pMenu->AddButton(116); // Move
    int btn_NewFolder = pMenu->AddButton(20309); // New Folder
    int btn_Size = pMenu->AddButton(13393); // Calculate Size

    int btn_Settings = pMenu->AddButton(5);			// Settings
    int btn_GoToRoot = pMenu->AddButton(20128);	// Go To Root
    int btn_Switch = pMenu->AddButton(523);     // switch media

    pMenu->EnableButton(btn_SelectAll, item >= 0);
    pMenu->EnableButton(btn_Rename, item >= 0 && CanRename(list) && !m_vecItems[list][item]->IsParentFolder());
    pMenu->EnableButton(btn_Delete, item >= 0 && CanDelete(list) && showEntry);
    pMenu->EnableButton(btn_Copy, item >= 0 && CanCopy(list) && showEntry);
    pMenu->EnableButton(btn_Move, item >= 0 && CanMove(list) && showEntry);
    pMenu->EnableButton(btn_NewFolder, CanNewFolder(list));
    pMenu->EnableButton(btn_Size, item >=0 && m_vecItems[list][item]->m_bIsFolder && !m_vecItems[list][item]->IsParentFolder());

    // position it correctly
    pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
    pMenu->DoModal();
    int btnid = pMenu->GetButton();
    if (btnid == btn_SelectAll)
    {
      OnSelectAll(list);
      bDeselect=false;
    }
    if (btnid == btn_Rename)
      OnRename(list);
    if (btnid == btn_Delete)
      OnDelete(list);
    if (btnid == btn_Copy)
      OnCopy(list);
    if (btnid == btn_Move)
      OnMove(list);
    if (btnid == btn_NewFolder)
      OnNewFolder(list);
    if (btnid == btn_Size)
    {
      // setup the progress dialog, and show it
      CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (progress)
      {
        progress->SetHeading(13394);
        for (int i=0; i < 3; i++)
          progress->SetLine(i, "");
        progress->StartModal();
      }

      //  Calculate folder size for each selected item
      for (int i=0; i<m_vecItems[list].Size(); ++i)
      {
        CFileItem* pItem=m_vecItems[list][i];
        if (pItem->m_bIsFolder && pItem->IsSelected())
        {
          __int64 folderSize = CalculateFolderSize(pItem->m_strPath, progress);
          if (folderSize >= 0)
          {
            pItem->m_dwSize = folderSize;
            if (folderSize == 0)
              pItem->SetLabel2(StringUtils::SizeToString(folderSize));
            else
              pItem->SetFileSizeLabel();
          }
        }
      }
      if (progress)
        progress->Close();
    }
    if (btnid == btn_Settings)
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MENU);
      return;
    }
    if (btnid == btn_GoToRoot)
    {
      Update(list,"");
      return;
    }
    if (btnid == btn_Switch)
    {
      CGUIDialogContextMenu::SwitchMedia("files", m_vecItems[list].m_strPath);
      return;
    }

    if (bDeselect && item >= 0 && item < m_vecItems[list].Size())
    { // deselect item as we didn't do anything
      m_vecItems[list][item]->Select(false);
    }
  }
}

// Highlights the item in the list under the cursor
// returns true if we should deselect the item, false otherwise
bool CGUIWindowFileManager::SelectItem(int list, int &item)
{
  // get the currently selected item in the list
  item = GetSelectedItem(list);

  // select the item if we need to
  if (item > -1 && !NumSelected(list) && !m_vecItems[list][item]->IsParentFolder())
  {
    m_vecItems[list][item]->Select(true);
    return true;
  }
  return false;
}

// recursively calculates the selected folder size
__int64 CGUIWindowFileManager::CalculateFolderSize(const CStdString &strDirectory, CGUIDialogProgress *pProgress)
{
  if (pProgress)
  { // update our progress control
    pProgress->Progress();
    pProgress->SetLine(1, strDirectory);
    if (pProgress->IsCanceled())
      return -1;
  }
  // start by calculating the size of the files in this folder...
  __int64 totalSize = 0;
  CFileItemList items;
  CVirtualDirectory rootDir;
  rootDir.SetShares(g_settings.m_vecMyFilesShares);
  rootDir.GetDirectory(strDirectory, items, false);
  for (int i=0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder && !items[i]->IsParentFolder()) // folder
    {
      __int64 folderSize = CalculateFolderSize(items[i]->m_strPath, pProgress);
      if (folderSize < 0) return -1;
      totalSize += folderSize;
    }
    else // file
      totalSize += items[i]->m_dwSize;
  }
  return totalSize;
}

bool CGUIWindowFileManager::DeleteItem(const CFileItem *pItem)
{
  if (!pItem) return false;
  CLog::Log(LOGDEBUG,"FileManager::DeleteItem: %s",pItem->GetLabel().c_str());

  // prompt user for confirmation of file/folder deletion
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CUtil::GetFileName(pItem->m_strPath));
    pDialog->SetLine(2, "");
    pDialog->DoModal();
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItem *pItemTemp = new CFileItem(*pItem);
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CGUIWindowFileManager *pFileManager = (CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES);
  if (pFileManager)
  {
    pFileManager->m_dlgProgress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pFileManager->ResetProgressBar(false);
    pFileManager->DoProcess(ACTION_DELETE, items, "");
    if (pFileManager->m_dlgProgress) pFileManager->m_dlgProgress->Close();
  }
  return true;
}

bool CGUIWindowFileManager::CopyItem(const CFileItem *pItem, const CStdString& strDirectory, bool bSilent, CGUIDialogProgress* pProgress)
{
  if (!pItem) return false;
  CLog::Log(LOGDEBUG,"FileManager::CopyItem: %s",pItem->GetLabel().c_str());

  // prompt user for confirmation of file/folder deletion
  if (!bSilent)
      if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(122),g_localizeStrings.Get(125),  CUtil::GetFileName(pItem->m_strPath), ""))
      return false;

  // Create a temporary item list containing the file/folder for deletion
  CFileItemList items;

  if (pItem->m_bIsFolder)
  {
    CDirectory::GetDirectory(pItem->m_strPath,items,"",false);
    for (int i=0;i<items.Size();++i)
      items[i]->Select(true);
  }
  else
  {
    CFileItem *pItemTemp = new CFileItem(*pItem);
    pItemTemp->Select(true);
    items.Add(pItemTemp);
  }

  bool bAllocated = false;
  CGUIWindowFileManager *pFileManager = (CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES);
  if (!pFileManager)
  {
    pFileManager = new CGUIWindowFileManager();
    bAllocated = true;
  }
  if (pFileManager)
  {
    pFileManager->m_dlgProgress = pProgress;
    pFileManager->ResetProgressBar(false);
    pFileManager->DoProcess(ACTION_COPY, items, strDirectory);
    if (pFileManager->m_dlgProgress) pFileManager->m_dlgProgress->Close();
  }
  if (bAllocated)
    delete pFileManager;

  return true;
}


void CGUIWindowFileManager::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    CURL url(pItem->m_strPath);
    const CStdString& strHostName=url.GetHostName();

    if (pItem->m_iDriveType!=SHARE_TYPE_REMOTE) //  Local shares incl. dvd drive
      idMessageText=15300;
    else if (url.GetProtocol()=="xbms" && strHostName.IsEmpty()) //  xbms server discover
      idMessageText=15302;
    else if (url.GetProtocol()=="smb" && strHostName.IsEmpty()) //  smb workgroup
      idMessageText=15303;
    else  //  All other remote shares
      idMessageText=15301;

    CGUIDialogOK::ShowAndGetInput(220, idMessageText, 0, 0);
  }
}

void CGUIWindowFileManager::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // disable the page spin controls
  // TODO: ListContainer - what to do here?
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIControl *spin = (CGUIControl *)GetControl(CONTROL_LEFT_LIST + 5000);
  if (spin) spin->SetVisible(false);
  spin = (CGUIControl *)GetControl(CONTROL_RIGHT_LIST + 5000);
  if (spin) spin->SetVisible(false);
#endif
}

void CGUIWindowFileManager::OnInitWindow()
{
  m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_dlgProgress) m_dlgProgress->SetHeading(126);

  for (int i = 0; i < 2; i++)
  {
    Update(i, m_Directory[i].m_strPath);
  }
  CGUIWindow::OnInitWindow();

  if (!bCheckShareConnectivity)
  {
    bCheckShareConnectivity = true; //reset
    CFileItem pItem;
    pItem.m_strPath=strCheckSharePath;
    pItem.m_bIsShareOrDrive = true;
    if (CUtil::IsHD(strCheckSharePath))
      pItem.m_iDriveType=SHARE_TYPE_LOCAL;
    else //we asume that this is a remote share else we can set SHARE_TYPE_UNKNOWN
      pItem.m_iDriveType=SHARE_TYPE_REMOTE;
    ShowShareErrorMessage(&pItem); //show the error message after window is loaded!
    Update(0,""); // reset view to root
  }
}

void CGUIWindowFileManager::SetInitialPath(const CStdString &path)
{
  // check for a passed destination path
  CStdString strDestination = path;
  m_rootDir.SetShares(*g_settings.GetSharesFromType("files"));
  if (!strDestination.IsEmpty())
  {
    CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
  }
  // otherwise, is this the first time accessing this window?
  else if (m_Directory[0].m_strPath == "?")
  {
    m_Directory[0].m_strPath = strDestination = g_stSettings.m_szDefaultFiles;
    CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
  }
  // try to open the destination path
  if (!strDestination.IsEmpty())
  {
    // open root
    if (strDestination.Equals("$ROOT"))
    {
      m_Directory[0].m_strPath = "";
      CLog::Log(LOGINFO, "  Success! Opening root listing.");
    }
    else
    {
      // default parameters if the jump fails
      m_Directory[0].m_strPath = "";

      bool bIsSourceName = false;
      VECSHARES shares;
      m_rootDir.GetShares(shares);
      int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsSourceName);
      if (iIndex > -1)
      {
        // set current directory to matching share
        if (bIsSourceName)
          m_Directory[0].m_strPath = shares[iIndex].strPath;
        else
          m_Directory[0].m_strPath = strDestination;
        CUtil::RemoveSlashAtEnd(m_Directory[0].m_strPath);
        CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());

        // outside call: check the share for connectivity
        bCheckShareConnectivity = Update(0, m_Directory[0].m_strPath);
        if(!bCheckShareConnectivity)
          strCheckSharePath = m_Directory[0].m_strPath;
      }
      else
      {
        CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
      }
    }
  }

  if (m_Directory[1].m_strPath == "?") m_Directory[1].m_strPath = "";
}

void CGUIWindowFileManager::ResetProgressBar(bool showProgress /*= true */)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(126);
    m_dlgProgress->SetLine(0, 0);
    m_dlgProgress->SetLine(1, 0);
    m_dlgProgress->SetLine(2, 0);
    m_dlgProgress->StartModal();
    m_dlgProgress->SetPercentage(0);
    m_dlgProgress->ShowProgressBar(showProgress);
  }
}

bool CGUIWindowFileManager::MoveItem(const CFileItem *pItem, const CStdString& strDirectory, bool bSilent, CGUIDialogProgress* pProgress)
{
  if (!pItem) return false;
  CLog::Log(LOGDEBUG,"FileManager::MoveItem: %s",pItem->GetLabel().c_str());

  // prompt user for confirmation of file/folder moving
  //if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(121),g_localizeStrings.Get(124),  CUtil::GetFileName(pItem->m_strPath), ""))	return false;
  if (!bSilent)
    if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(121),g_localizeStrings.Get(124),  CUtil::GetFileName(pItem->m_strPath), ""))
      return false;

  // Create a temporary item list containing the file/folder for deletion
  CFileItemList items;

  if (pItem->m_bIsFolder)
  {
    CDirectory::GetDirectory(pItem->m_strPath,items,"",false);
    for (int i=0;i<items.Size();++i)
      items[i]->Select(true);
  }
  else
  {
    CFileItem *pItemTemp = new CFileItem(*pItem);
    items.Add(pItemTemp);
  }

  bool bAllocated = false;
  CGUIWindowFileManager *pFileManager = (CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES);
  if (!pFileManager)
  {
    pFileManager = new CGUIWindowFileManager();
    bAllocated = true;
  }
  if (pFileManager)
  {
    pFileManager->m_dlgProgress = pProgress;
    pFileManager->ResetProgressBar(false);
    pFileManager->DoProcess(ACTION_MOVE, items, strDirectory);
    if (pFileManager->m_dlgProgress) pFileManager->m_dlgProgress->Close();
  }
  if (bAllocated)
    delete pFileManager;

  return true;
}
