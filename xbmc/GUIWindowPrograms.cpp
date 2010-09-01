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

#include "system.h"
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
#include "FileItem.h"
#include "Settings.h"
#include "LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

using namespace XFILE;

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
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // is this the first time accessing this window?
      if (m_vecItems->m_strPath == "?" && message.GetStringParam().IsEmpty())
        message.SetStringParam(g_settings.m_defaultProgramSource);

      m_database.Open();

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
      CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
    }
    else
    {
      if (item->IsXBE() || item->IsShortCut())
      {
        CStdString strLaunch = g_localizeStrings.Get(518); // Launch
        buttons.Add(CONTEXT_BUTTON_LAUNCH, strLaunch);

        if (g_passwordManager.IsMasterLockUnlocked(false) || g_settings.GetCurrentProfile().canWriteDatabases())
        {
          if (item->IsShortCut())
            buttons.Add(CONTEXT_BUTTON_RENAME, 16105); // rename
          else
            buttons.Add(CONTEXT_BUTTON_RENAME, 520); // edit xbe title
        }
      }

      if (item->IsPlugin() || item->m_strPath.Left(9).Equals("script://") || m_vecItems->IsPlugin())
        buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);

      buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128); // Go to Root
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPrograms::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();

  if (item && m_vecItems->IsVirtualDirectoryRoot())
  {
    if (CGUIDialogContextMenu::OnContextButton("programs", item, button))
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

#ifdef HAS_DVD_DRIVE
  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDisc();
#endif

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
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  // don't allow the view state to change these
  if (strDirectory.Left(9).Equals("addons://"))
  {
    for (int i=0;i<items.Size();++i)
    {
      items[i]->SetLabel2(items[i]->GetProperty("Addon.Version"));
      items[i]->SetLabelPreformated(true);
    }
  }

  return true;
}

CStdString CGUIWindowPrograms::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/executable/";
    
  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex > -1)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item,"programs"))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}
