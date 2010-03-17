/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "utils/AddonManager.h"
#include "GUIWindowAddonBrowser.h"
#include "GUISpinControlEx.h"
#include "GUIControlGroupList.h"
#include "Settings.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "FileItem.h"

#define CONTROL_ADDONSLIST                 2
#define CATEGORY_GROUP_ID                  3
#define CONTROL_DEFAULT_CATEGORY_BUTTON   10
#define CONTROL_HEADING_LABEL            411
#define CONTROL_START_BUTTONS           -100

using namespace ADDON;

CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIMediaWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnAction(const CAction &action)
{
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowAddonBrowser::OnMessage(CGUIMessage& message)
{
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowAddonBrowser::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = (*m_vecItems)[iItem];
  CStdString strPath = pItem->m_strPath;

  // check if user is allowed to open this window
  if (g_settings.GetCurrentProfile().addonmanagerLocked() && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

  AddonPtr addon;
  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  if (CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), addon, type, false))
  {
    if (addon->Disabled())
      CAddonMgr::Get()->EnableAddon(addon);
    else
      CAddonMgr::Get()->DisableAddon(addon);
    Update(m_vecItems->m_strPath);
  }

  return true;
}

void CGUIWindowAddonBrowser::GetContextButtons(int iItem, CContextButtons& buttons)
{
  // check if user is allowed to open this window
  if (g_settings.GetCurrentProfile().addonmanagerLocked() && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  CFileItemPtr pItem = m_vecItems->Get(iItem);

  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), addon, type, false))
    return;

  if (!addon->Disabled() && addon->HasSettings())
    buttons.Add(CONTEXT_BUTTON_SETTINGS, 24020);
}

bool CGUIWindowAddonBrowser::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), addon, type))
    return false;

  if (button == CONTEXT_BUTTON_SETTINGS)
  {
    CGUIDialogAddonSettings::ShowAndGetInput(addon);
    return true;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowAddonBrowser::GetDirectory(const CStdString& strDirectory, CFileItemList& items)
{
printf("getdir\n");
  if (strDirectory.IsEmpty())
  {
    CFileItemPtr item(new CFileItem("test"));
    item->m_strPath = "addons://scrapers"; 
    items.Add(item);
    return true;
  }

  return CGUIMediaWindow::GetDirectory(strDirectory,items);
}

