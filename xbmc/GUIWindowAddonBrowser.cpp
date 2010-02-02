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

#define CONTROL_ADDONSLIST      2
#define CONTROL_HEADING_LABEL   411
#define CONTROL_GETADDONS       415

using namespace ADDON;

CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
  m_vecItems = new CFileItemList;
  m_confirmed = false;
  m_type = ADDON_SCRAPER;
  m_content = CONTENT_MOVIES;
  m_getAddons = false;
  m_changed = false;
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnAction(const CAction &action)
{
  if (action.actionId == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.actionId == ACTION_CONTEXT_MENU || action.actionId == ACTION_MOUSE_RIGHT_CLICK)
  {
    int iItem = GetSelectedItem();
    return OnContextMenu(iItem);
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowAddonBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      ClearListItems();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_ADDONSLIST)  // list control
      {
        int iItem = GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
          return true;
        }
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          if(OnContextMenu(iItem))
          { // something changed, disable cancel button
            m_changed = true;
          }
          return true;
        }
      }
      else if (message.GetSenderId() == CONTROL_GETADDONS)
      {
        // check if user is allowed to open this window
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;
        OnGetAddons(m_type);
        return true;
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowAddonBrowser::ClearListItems()
{
  m_vecItems->Clear(); // will clean up everything
}

void CGUIWindowAddonBrowser::OnInitWindow()
{
  Update();
  CGUIWindow::OnInitWindow();
}

int CGUIWindowAddonBrowser::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ADDONSLIST);
  g_windowManager.SendMessage(msg);

  return msg.GetParam1();
}

bool CGUIWindowAddonBrowser::SelectItem(int select)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_ADDONSLIST, select);
  return g_windowManager.SendMessage(msg);
}

void CGUIWindowAddonBrowser::OnSort()
{
  m_vecItems->Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}

void CGUIWindowAddonBrowser::Update()
{
  int selected = GetSelectedItem();
  m_vecItems->Clear();

  VECADDONS addons;
  CAddonMgr::Get()->GetAllAddons(addons, false);

  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    CFileItemPtr pItem(new CFileItem(addon->UUID(), false));
    pItem->SetLabel(addon->Name());
    pItem->SetProperty("Addon.UUID", addon->UUID());
    pItem->SetProperty("Addon.Type", TranslateType(addon->Type()));
    pItem->SetProperty("Addon.Disabled", addon->Disabled());
    pItem->SetProperty("Addon.Name", addon->Name());
    pItem->SetProperty("Addon.Version", addon->Version().Print());
    pItem->SetProperty("Addon.Summary", addon->Summary());
    pItem->SetProperty("Addon.Description", addon->Description());
    pItem->SetProperty("Addon.Creator", addon->Author());
    pItem->SetProperty("Addon.Disclaimer", addon->Disclaimer());
    pItem->SetProperty("Addon.Rating", addon->Stars());
    pItem->SetThumbnailImage(addon->Icon());
    m_vecItems->Add(pItem);
  }

  OnSort();
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_ADDONSLIST, 0, 0, m_vecItems);
  OnMessage(msg);

  if (selected != 0)
    SelectItem(selected);
}

void CGUIWindowAddonBrowser::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return ;
  CFileItemPtr pItem = (*m_vecItems)[iItem];
  CStdString strPath = pItem->m_strPath;

  if (m_getAddons)
  {
    // get a pointer to the addon in question
    AddonPtr addon;
    TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
    if (CAddonMgr::Get()->GetAddon(type, pItem->GetProperty("Addon.UUID"), addon))
    {
      CStdString disclaimer = pItem->GetProperty("Addon.Disclaimer");
      if (!disclaimer.empty())
      {
         if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(23058), pItem->GetProperty("Addon.Name"), disclaimer, g_localizeStrings.Get(23059)))
           return;
      }

      // now enable this addon (will show up on parent dialog)
      CAddonMgr::Get()->EnableAddon(addon);
      m_confirmed = true;
      Close(false);
    }
    else
    {
      // shouldn't happen as we just retrieved this uuid from g_settings
      CLog::Log(LOGERROR, "GetAddons: Can't determine Addon by UUID");
    }
  }
  else
  {
    // check if user is allowed to open this window
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    AddonPtr addon;
    TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
    if (CAddonMgr::Get()->GetAddon(type, pItem->GetProperty("Addon.UUID"), addon))
    {
      if (addon->Disabled())
      {
        CAddonMgr::Get()->EnableAddon(addon);
        Update();

      }
      else
        CGUIDialogAddonSettings::ShowAndGetInput(addon);
    }
  }
}

bool CGUIWindowAddonBrowser::ManageAddons(const ADDON::TYPE &type, const CONTENT_TYPE &content, const bool viewActive/*=true*/)
{
  /*// Create a new addonbrowser window
  CGUIWindowAddonBrowser *browser = new CGUIWindowAddonBrowser();
  if (!browser) return false;

  // Add it to our window manager
  g_windowManager.AddUniqueInstance(browser);

  // determine the correct heading
  CStdString heading;
  if (!viewActive)
    heading = g_localizeStrings.Get(23002); // "Available Add-ons"
  else
    heading = g_localizeStrings.Get(23060 + type); // Name is calculated by type! //TODO fix this

  // check if user is allowed to open this window
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    heading = heading + " (" + g_localizeStrings.Get(20166) + ")";

  // finalize the window and display
  browser->SetHeading(heading);
  browser->SetType(type);
  browser->SetContent(content);
  browser->SetActiveOnly(viewActive);
  browser->DoModal();
  bool confirmed = browser->IsConfirmed();

  g_windowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;*/
  return true;
}

void CGUIWindowAddonBrowser::SetType(const ADDON::TYPE &type)
{
  m_type = type;
}

void CGUIWindowAddonBrowser::SetContent(const CONTENT_TYPE &content)
{
  m_content = content;
}

void CGUIWindowAddonBrowser::OnGetAddons(const ADDON::TYPE &type)
{
/*  // present dialog with available addons
  if (ManageAddons(type, m_content, false))
  {
    // need to update the list of installed addons
    Update();
    // we made changes
    m_changed = true;
  }*/
}

bool CGUIWindowAddonBrowser::OnContextMenu(int iItem)
{
  // check if user is allowed to open this window
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

  CGUIDialogContextMenu* pMenu = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu)
    return false;

  pMenu->Initialize();
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(type, pItem->GetProperty("Addon.UUID"), addon))
    return false;

  int iSettingsLabel = 23008;
  int iDisableLabel = 23009;
  int iEnableLabel = 23010;

  int btn_Disable = -1;
  int btn_Enable = -1;
  int btn_Settings = -1;

  if (addon->Disabled())
    btn_Enable = pMenu->AddButton(iEnableLabel);
  else
  {
    btn_Disable = pMenu->AddButton(iDisableLabel);
    btn_Settings = pMenu->AddButton(iSettingsLabel);
  }


  pMenu->CenterWindow();
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid == -1)
    return false;

  if (btnid == btn_Settings)
  {
    CGUIDialogAddonSettings::ShowAndGetInput(addon);
    return true;
  }
  else if (btnid == btn_Disable)
  {
    CAddonMgr::Get()->DisableAddon(addon);
    Update();
    return true;
  }
  else if (btnid == btn_Enable)
  {
    CAddonMgr::Get()->EnableAddon(addon);
    Update();
    return true;
  }
  return false;
}

