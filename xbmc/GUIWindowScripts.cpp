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
#include "GUIWindowScripts.h"
#include "Util.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "GUIWindowScriptsInfo.h"
#include "GUIWindowManager.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "addons/AddonManager.h"
#include "GUIDialogAddonSettings.h"
#include "Settings.h"
#include "LocalizeStrings.h"
#if defined(__APPLE__)
#include "SpecialProtocol.h"
#include "CocoaInterface.h"
#endif
#include "utils/FileUtils.h"
#include "AddonDatabase.h"

using namespace XFILE;
using namespace ADDON;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LABELFILES        12

CGUIWindowScripts::CGUIWindowScripts()
    : CGUIMediaWindow(WINDOW_SCRIPTS, "MyScripts.xml")
{
  m_bViewOutput = false;
  m_scriptSize = 0;
}

CGUIWindowScripts::~CGUIWindowScripts()
{
}

bool CGUIWindowScripts::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    OnInfo();
    return true;
  }
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowScripts::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      if (m_vecItems->m_strPath == "?")
        m_vecItems->m_strPath = g_settings.GetScriptsFolder();
      return CGUIMediaWindow::OnMessage(message);
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowScripts::Update(const CStdString &strDirectory)
{
  // Look if baseclass can handle it
  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

#ifdef HAS_PYTHON
  /* check if any python scripts are running. If true, place "(Running)" after the item.
   * since stopping a script can take up to 10 seconds or more,we display 'stopping'
   * after the filename for now.
   */
  int iSize = g_pythonParser.ScriptsSize();
  for (int i = 0; i < iSize; i++)
  {
    int id = g_pythonParser.GetPythonScriptId(i);
    if (g_pythonParser.isRunning(id))
    {
      const char* filename = g_pythonParser.getFileName(id);

      for (int i = 0; i < m_vecItems->Size(); i++)
      {
        CFileItemPtr pItem = m_vecItems->Get(i);
        if (pItem->m_strPath == filename)
        {
          CStdString runningLabel = pItem->GetLabel() + " (";
          if (g_pythonParser.isStopping(id))
            runningLabel += g_localizeStrings.Get(23053) + ")";
          else
            runningLabel += g_localizeStrings.Get(23054) + ")";
          pItem->SetLabel(runningLabel);
        }
      }
    }
  }
#endif

  return true;
}

bool CGUIWindowScripts::OnPlayMedia(int iItem)
{
  CFileItemPtr pItem=m_vecItems->Get(iItem);
  CStdString strPath = pItem->m_strPath;

#if defined(__APPLE__)
  if (CUtil::GetExtension(pItem->m_strPath) == ".applescript")
  {
    CStdString osxPath = CSpecialProtocol::TranslatePath(pItem->m_strPath);
    Cocoa_DoAppleScriptFile(osxPath.c_str());
    return true;
  }
#endif

#ifdef HAS_PYTHON
  /* execute script...
    * if script is already running do not run it again but stop it.
    */
  int id = g_pythonParser.getScriptId(strPath);
  if (id != -1)
  {
    /* if we are here we already know that this script is running.
      * But we will check it again to be sure :)
      */
    if (g_pythonParser.isRunning(id))
    {
      g_pythonParser.stopScript(id);

      // update items
      int selectedItem = m_viewControl.GetSelectedItem();
      Update(m_vecItems->m_strPath);
      m_viewControl.SetSelectedItem(selectedItem);
      return true;
    }
  }
  g_pythonParser.evalFile(strPath);
#endif

  return true;
}

void CGUIWindowScripts::OnInfo()
{
  CGUIWindowScriptsInfo* pDlgInfo = (CGUIWindowScriptsInfo*)g_windowManager.GetWindow(WINDOW_SCRIPTS_INFO);
  if (pDlgInfo) pDlgInfo->DoModal();
}

void CGUIWindowScripts::FrameMove()
{
#ifdef HAS_PYTHON
  // update control_list / control_thumbs if one or more scripts have stopped / started
  if (g_pythonParser.ScriptsSize() != m_scriptSize)
  {
    int selectedItem = m_viewControl.GetSelectedItem();
    Update(m_vecItems->m_strPath);
    m_viewControl.SetSelectedItem(selectedItem);
    m_scriptSize = g_pythonParser.ScriptsSize();
  }
#endif

  CGUIWindow::FrameMove();
}

bool CGUIWindowScripts::GetDirectory(const CStdString& strDirectory, CFileItemList& items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_SCRIPT,addons);
  
  items.ClearItems();
  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    CFileItemPtr pItem(new CFileItem(addon->Path()+addon->LibName(),false));
    pItem->SetLabel(addon->Name());
    pItem->SetLabel2(addon->Summary());
    pItem->SetThumbnailImage(addon->Icon());
    CAddonDatabase::SetPropertiesFromAddon(addon,pItem);
    items.Add(pItem);
  }  	

  return true;
}

void CGUIWindowScripts::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);

  // add script settings item
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();
  if (item && item->IsPythonScript())
  {
    CStdString path, filename;
    CUtil::Split(item->m_strPath, path, filename);
    ADDON::AddonPtr script;
    if (ADDON::CAddonMgr::Get().GetAddon(item->m_strPath, script, ADDON::ADDON_SCRIPT))
    {
      if (script->HasSettings())
      {
        buttons.Add(CONTEXT_BUTTON_SCRIPT_SETTINGS, 1049);
      }
    }
  }

  buttons.Add(CONTEXT_BUTTON_INFO, 654);
  buttons.Add(CONTEXT_BUTTON_DELETE, 117);
}

bool CGUIWindowScripts::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (button == CONTEXT_BUTTON_INFO)
  {
    OnInfo();
    return true;
  }
  else if (button == CONTEXT_BUTTON_SCRIPT_SETTINGS)
  {
    CStdString path, filename;
    CUtil::Split(m_vecItems->Get(itemNumber)->m_strPath, path, filename);
    ADDON::AddonPtr script;
    if (ADDON::CAddonMgr::Get().GetAddon(m_vecItems->Get(itemNumber)->m_strPath, script, ADDON::ADDON_SCRIPT))
    {
      if (CGUIDialogAddonSettings::ShowAndGetInput(script))
        Update(m_vecItems->m_strPath);
    }
    return true;
  }
  else if (button == CONTEXT_BUTTON_DELETE)
  {
    CStdString path;
    CUtil::GetDirectory(m_vecItems->Get(itemNumber)->m_strPath,path);
    CFileItemPtr item2(new CFileItem(path,true));
    if (CFileUtils::DeleteItem(item2))
      Update(m_vecItems->m_strPath);

    return true;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

