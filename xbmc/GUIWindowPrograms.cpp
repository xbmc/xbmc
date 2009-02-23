/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowPrograms.h"
#include "Util.h"
#include "Shortcut.h"
#include "FileSystem/HDDirectory.h"
#include "GUIPassword.h"
#include "GUIDialogMediaSource.h"
#include "Autorun.h"
#include "utils/LabelFormatter.h"
#include "Autorun.h"
#include "Profile.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileSystem/RarManager.h"
#include "FileItem.h"

using namespace XFILE;
using namespace DIRECTORY;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_LABELFILES    12

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
  m_thumbLoader.SetObserver(this);
  m_dlgProgress = NULL;
  m_rootDir.AllowNonLocalSources(false); // no nonlocal shares for this window please
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_iRegionSet = 0;
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        // reset directory path, as we have effectively cleared it here
        m_history.ClearPathHistory();
      }
      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if (m_vecItems->m_strPath == "?" && strDestination.IsEmpty())
      {
        m_vecItems->m_strPath = strDestination = g_settings.m_defaultProgramSource;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      m_database.Open();
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems->m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems->m_strPath = "";

          bool bIsSourceName = false;
          SetupShares();
          VECSOURCES shares;
          m_rootDir.GetSources(shares);
          int iIndex = CUtil::GetMatchingSource(strDestination, shares, bIsSourceName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"programs"))
              {
                m_vecItems->m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsSourceName)
                m_vecItems->m_strPath=shares[iIndex].strPath;
              else
                m_vecItems->m_strPath=strDestination;
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid source!", strDestination.c_str());
          }
        }
        SetHistoryForPath(m_vecItems->m_strPath);
      }

      return CGUIMediaWindow::OnMessage(message);
    }
  break;

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_BTNSORTBY)
      {
        // need to update shortcuts manually
        if (CGUIMediaWindow::OnMessage(message))
        {
          LABEL_MASKS labelMasks;
          m_guiState->GetSortMethodLabelMasks(labelMasks);
          CLabelFormatter formatter("", labelMasks.m_strLabel2File);
          for (int i=0;i<m_vecItems->Size();++i)
          {
            CFileItemPtr item = m_vecItems->Get(i);
            if (item->IsShortCut())
              formatter.FormatLabel2(item.get());
          }
          return true;
        }
        else
          return false;
      }
      if (m_viewControl.HasControl(message.GetSenderId()))  // list/thumb control
      {
        if (message.GetParam1() == ACTION_PLAYER_PLAY)
        {
          OnPlayMedia(m_viewControl.GetSelectedItem());
          return true;
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPrograms::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item && !item->GetPropertyBOOL("pluginreplacecontextitems"))
  {
    if ( m_vecItems->IsVirtualDirectoryRoot() )
    {
      // get the usual shares
      CMediaSource *share = CGUIDialogContextMenu::GetShare("programs", item.get());
      CGUIDialogContextMenu::GetContextButtons("programs", share, buttons);
    }
    else
    {
      if (item->IsXBE() || item->IsShortCut())
      {
        CStdString strLaunch = g_localizeStrings.Get(518); // Launch
        buttons.Add(CONTEXT_BUTTON_LAUNCH, strLaunch);

        if (g_passwordManager.IsMasterLockUnlocked(false) || g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases())
        {
          if (item->IsShortCut())
            buttons.Add(CONTEXT_BUTTON_RENAME, 16105); // rename
          else
            buttons.Add(CONTEXT_BUTTON_RENAME, 520); // edit xbe title
        }
      }
      buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128); // Go to Root
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
  if (item && !item->GetPropertyBOOL("pluginreplacecontextitems"))
    buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);      // Settings
}

bool CGUIWindowPrograms::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();

  if (item && m_vecItems->IsVirtualDirectoryRoot())
  {
    CMediaSource *share = CGUIDialogContextMenu::GetShare("programs", item.get());
    if (CGUIDialogContextMenu::OnContextButton("programs", share, button))
    {
      Update("");
      return true;
    }
  }
  switch (button)
  {
  case CONTEXT_BUTTON_RENAME:
    {
      CStdString strDescription;
      CShortcut cut;
      if (item->IsShortCut())
      {
        cut.Create(item->m_strPath);
        strDescription = cut.m_strLabel;
      }
      else
        strDescription = item->GetLabel();

      if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, g_localizeStrings.Get(16008), false))
      {
        if (item->IsShortCut())
        {
          cut.m_strLabel = strDescription;
          cut.Save(item->m_strPath);
        }
        else
        {
          // SetXBEDescription will truncate to 40 characters.
          //CUtil::SetXBEDescription(item->m_strPath,strDescription);
          //m_database.SetDescription(item->m_strPath,strDescription);
        }
        Update(m_vecItems->m_strPath);
      }
      return true;
    }

  case CONTEXT_BUTTON_SETTINGS:
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS);
    return true;

  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;

  case CONTEXT_BUTTON_LAUNCH:
    OnClick(itemNumber);
    return true;

  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(*m_vecItems);
  return true;
}

bool CGUIWindowPrograms::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("programs"))
    {
      Update("");
      return true;
    }
    return false;
  }

  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDisc();

  if (pItem->m_bIsFolder) return false;

  return false;
}

int CGUIWindowPrograms::GetRegion(int iItem, bool bReload)
{
  // TODO?
  return 0;
}

bool CGUIWindowPrograms::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  bool bFlattened=false;
  if (CUtil::IsDVD(strDirectory))
  {
    CStdString strPath;
    CUtil::AddFileToFolder(strDirectory,"default.xbe",strPath);
    if (CFile::Exists(strPath)) // flatten dvd
    {
      CFileItemPtr item(new CFileItem("default.xbe"));
      item->m_strPath = strPath;
      items.Add(item);
      items.m_strPath=strDirectory;
      bFlattened = true;
    }
  }
  if (!bFlattened)
    if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
      return false;

  if (items.IsVirtualDirectoryRoot())
    return true;

  // flatten any folders
  m_database.BeginTransaction();
  DWORD dwTick=timeGetTime();
  bool bProgressVisible = false;
  for (int i = 0; i < items.Size(); i++)
  {
    CStdString shortcutPath;
    CFileItemPtr item = items[i];
    if (!bProgressVisible && timeGetTime()-dwTick>1500 && m_dlgProgress)
    { // tag loading takes more then 1.5 secs, show a progress dialog
      m_dlgProgress->SetHeading(189);
      m_dlgProgress->SetLine(0, 20120);
      m_dlgProgress->SetLine(1,"");
      m_dlgProgress->SetLine(2, item->GetLabel());
      m_dlgProgress->StartModal();
      bProgressVisible = true;
    }
    if (bProgressVisible)
    {
      m_dlgProgress->SetLine(2,item->GetLabel());
      m_dlgProgress->Progress();
    }

    if (item->m_bIsFolder && !item->IsParentFolder())
    { // folder item - let's check for a default.xbe file, and flatten if we have one
      CStdString defaultXBE;
      CUtil::AddFileToFolder(item->m_strPath, "default.xbe", defaultXBE);
      if (CFile::Exists(defaultXBE))
      { // yes, format the item up
        item->m_strPath = defaultXBE;
        item->m_bIsFolder = false;
      }
    }
    else if (item->IsShortCut())
    { // resolve the shortcut to set it's description etc.
      // and save the old shortcut path (so we can reassign it later)
      CShortcut cut;
      if (cut.Create(item->m_strPath))
      {
        shortcutPath = item->m_strPath;
        item->m_strPath = cut.m_strPath;
        item->SetThumbnailImage(cut.m_strThumb);

        LABEL_MASKS labelMasks;
        m_guiState->GetSortMethodLabelMasks(labelMasks);
        CLabelFormatter formatter("", labelMasks.m_strLabel2File);
        if (!cut.m_strLabel.IsEmpty())
        {
          item->SetLabel(cut.m_strLabel);
          struct __stat64 stat;
          if (CFile::Stat(item->m_strPath,&stat) == 0)
            item->m_dwSize = stat.st_size;

          formatter.FormatLabel2(item.get());
          item->SetLabelPreformated(true);
        }
      }
    }
    if (!shortcutPath.IsEmpty())
      item->m_strPath = shortcutPath;
  }
  m_database.CommitTransaction();
  // set the cached thumbs
  items.SetThumbnailImage("");
  items.SetCachedProgramThumbs();
  items.SetCachedProgramThumb();
  if (!items.HasThumbnail())
    items.SetUserProgramThumb();

  if (bProgressVisible)
    m_dlgProgress->Close();

  return true;
}

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
void CGUIWindowPrograms::OnWindowLoaded()
{
  CGUIMediaWindow::OnWindowLoaded();
  for (int i = 100; i < 110; i++)
  {
    SET_CONTROL_HIDDEN(i);
  }
}
#endif
