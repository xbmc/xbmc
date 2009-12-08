/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "StringUtils.h"
#include "utils/log.h"
#include "utils/Addon.h"
#include "GUIDialogAddonBrowser.h"
#include "GUISpinControlEx.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"
#include "GUIEditControl.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "ScraperSettings.h"
#include "LocalizeStrings.h"

#define CONTROL_LIST            450
#define CONTROL_HEADING_LABEL   411
#define CONTROL_LABEL_PATH      412
#define CONTROL_OK              413
#define CONTROL_CANCEL          414
#define CONTROL_ADDONS          415

using namespace ADDON;

CGUIDialogAddonBrowser::CGUIDialogAddonBrowser(void)
: CGUIDialog(WINDOW_DIALOG_ADDON_BROWSER, "DialogAddonBrowser.xml")
{
  m_vecItems = new CFileItemList;
  m_confirmed = false;
  m_changed = false;
  m_loadOnDemand = true;
}

CGUIDialogAddonBrowser::~CGUIDialogAddonBrowser()
{
}

bool CGUIDialogAddonBrowser::OnAction(const CAction &action)
{
  if (action.id == ACTION_CONTEXT_MENU || action.id == ACTION_MOUSE_RIGHT_CLICK)
  {
    int iItem = m_viewControl.GetSelectedItem();
    return OnContextMenu(iItem);
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogAddonBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      ClearFileItems();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      Update();
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
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnContextMenu(iItem);
          return true;
        }
      }
      else if (message.GetSenderId() == CONTROL_CANCEL)
      {
        Close();
        return true;
      }
      else if (message.GetSenderId() == CONTROL_ADDONS)
      {
        // check if user is allowed to open this window
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;
        OnGetAddons(m_type);
        return true;
      }
      else if (message.GetSenderId() == CONTROL_OK)
      {
        m_confirmed = true;
        Close();
        return true;
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && (DWORD) m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogAddonBrowser::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems->Clear(); // will clean up everything
}

void CGUIDialogAddonBrowser::OnSort()
{
  m_vecItems->Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}

void CGUIDialogAddonBrowser::Update()
{
  m_vecItems->Clear();

  VECADDONS *addons;
  if (m_getAddons)
  {
    addons = g_addonmanager.GetAllAddons();
  }
  else
  {
    addons = g_addonmanager.GetAddonsFromType(m_type);
    if (addons == NULL)
      return;
  }

  for (unsigned i=0; i < addons->size(); i++)
  {
    CAddon &addon = (*addons)[i];

    if (addon.Type() != m_type)
        continue;

    if (m_getAddons)
    { // don't show addons that are enabled
      //TODO add-on manager should do all this
      bool skip(false);
      VECADDONS *addons = g_addonmanager.GetAddonsFromType(m_type);
      for (unsigned i = 0; i < addons->size(); i++)
      {
        if ((*addons)[i].UUID() == addon.UUID())
          skip = true;
      }
      if (skip)
        continue;
    }

    CFileItemPtr pItem(new CFileItem(addon.Path(), false));
    pItem->SetProperty("Addon.GUID", addon.UUID());
    pItem->SetProperty("Addon.parentGUID", addon.Parent());
    pItem->SetProperty("Addon.Name", addon.Name());
    pItem->SetProperty("Addon.Summary", addon.Summary());
    pItem->SetProperty("Addon.Description", addon.Description());
    pItem->SetProperty("Addon.Creator", addon.Author());
    pItem->SetProperty("Addon.Disclaimer", addon.Disclaimer());
    pItem->SetProperty("Addon.m_strLibName", addon.LibName());
    pItem->SetProperty("Addon.Rating", addon.Stars());
    pItem->SetThumbnailImage(addon.Icon());
    m_vecItems->Add(pItem);
  }
  m_vecItems->FillInDefaultIcons();
  OnSort();

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  m_viewControl.SetSelectedItem(0);
}

void CGUIDialogAddonBrowser::Render()
{
  CONTROL_ENABLE_ON_CONDITION(CONTROL_ADDONS, !m_getAddons);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_OK, m_changed && !m_getAddons);

  CGUIDialog::Render();
}

void CGUIDialogAddonBrowser::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return ;
  CFileItemPtr pItem = (*m_vecItems)[iItem];
  CStdString strPath = pItem->m_strPath;

  if (m_getAddons)
  {
    /* need to determine which addon from allAddons this and add to AddonType specific vector */
    VECADDONS *addons = g_addonmanager.GetAddonsFromType(m_type);
    CAddon addon;
    if (g_addonmanager.GetAddonFromGUID(pItem->GetProperty("Addon.GUID"), addon))
    {
      CStdString disclaimer = pItem->GetProperty("Addon.Disclaimer");
      if (disclaimer.size() > 0)
      {
         CGUIDialogYesNo* pDialog = new CGUIDialogYesNo();
         if (!pDialog->ShowAndGetInput(g_localizeStrings.Get(23058), pItem->GetProperty("Addon.Name"), disclaimer, g_localizeStrings.Get(23059)))
           return;
      }

      // add the addon to g_settings, not saving to addons.xml until parent dialog is confirmed
      addons->push_back(addon);
      m_confirmed = true;
      Close(false);
    }
    else
    {
      // shouldn't happen as we just retrieved this guid from g_settings
      CLog::Log(LOGERROR, "Addons: Can't determine Addon by GUID");
    }
  }
  else
  {
    // check if user is allowed to open this window
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    if (m_type != ADDON_SCRAPER)
    {
      /* open up settings dialog */
      CAddon addon;
      if (g_addonmanager.GetAddonFromGUID(pItem->GetProperty("Addon.GUID"), addon))
        CGUIDialogAddonSettings::ShowAndGetInput(addon);
    }
  }
}

void CGUIDialogAddonBrowser::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
  // set the page spin controls to hidden
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIControl *spin = (CGUIControl *)GetControl(CONTROL_LIST + 5000);
  if (spin) spin->SetVisible(false);
#endif

  // request available addons update
  g_addonmanager.UpdateAddons();
}

void CGUIDialogAddonBrowser::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIDialogAddonBrowser::SetHeading(const CStdString &heading)
{
  Initialize();
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, heading);
}

bool CGUIDialogAddonBrowser::ShowAndGetAddons(const TYPE &type, const bool viewActive)
{
  // Create a new addonbrowser window
  CGUIDialogAddonBrowser *browser = new CGUIDialogAddonBrowser();
  if (!browser) return false;

  // Add it to our window manager
  g_windowManager.AddUniqueInstance(browser);

  // determine the correct heading
  CStdString heading;
  if (!viewActive)
    heading = g_localizeStrings.Get(23002); // "Available Add-ons"
  else
    heading = g_localizeStrings.Get(23060 + type); // Name is calculated by type!

  // check if user is allowed to open this window
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    heading = heading + " (" + g_localizeStrings.Get(20166) + ")";

  // finalize the window and display
  browser->SetHeading(heading);
  browser->SetAddonType(type);
  browser->SetAddNew(viewActive);
  browser->DoModal();
  bool confirmed = browser->IsConfirmed();

  g_windowManager.Remove(browser->GetID());
  delete browser;
  return confirmed;
}

void CGUIDialogAddonBrowser::SetAddonType(const TYPE &type)
{
  m_type = type;
}

void CGUIDialogAddonBrowser::OnGetAddons(const TYPE &type)
{
  // switch context to available addons
  // creates a new addonbrowser window
  CGUIDialogAddonBrowser *browser = new CGUIDialogAddonBrowser();
  if (!browser) return;

  // Add it to our window manager
  g_windowManager.AddUniqueInstance(browser);

  // present dialog with available addons
  if (ShowAndGetAddons(type, false))
  {
    // need to update the list of installed addons
    Update();
    // we made changes
    m_changed = true;
  }
}

bool CGUIDialogAddonBrowser::OnContextMenu(int iItem)
{
  // check if user is allowed to open this window
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].addonmanagerLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

  // disable context menu for available addons for now
  if (m_getAddons)
    return true;

  CGUIDialogContextMenu* pMenu = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu)
    return false;

  pMenu->Initialize();
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  int iSettingsLabel = 23008;
  int iReUseLabel = 23083;
  int iRemoveLabel = 23009;

  int btn_Settings = -1;
  int btn_ReUse = -1;
  if (m_type != ADDON_SCRAPER)
    btn_Settings = pMenu->AddButton(iSettingsLabel);
  if (m_type == ADDON_PVRDLL && pItem->GetProperty("Addon.parentGUID").IsEmpty())
    btn_ReUse = pMenu->AddButton(iReUseLabel);
  int btn_Remove = pMenu->AddButton(iRemoveLabel);

  pMenu->CenterWindow();
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid == btn_Settings)
  { // present addon settings dialog
    CAddon addon;
    if (g_addonmanager.GetAddonFromGUID(pItem->GetProperty("Addon.GUID"), addon))
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
    return true;
  }
  else if (btnid == btn_ReUse)
  {
    /* need to determine which addon from allAddons this and add to AddonType specific vector */
    VECADDONS *addons = g_addonmanager.GetAddonsFromType(m_type);
    CAddon addon_parent;
    if (g_addonmanager.GetAddonFromGUID(pItem->GetProperty("Addon.GUID"), addon_parent))
    {
      CAddon addon_child;
      if (CAddon::CreateChildAddon(addon_parent, addon_child))
      {
        // add the addon to g_settings, not saving to addons.xml until parent dialog is confirmed
        addons->push_back(addon_child);
        g_addonmanager.SaveVirtualAddon(addon_child);
        m_changed = true;
        Update();
        return true;
      }
    }
  }
  else if (btnid == btn_Remove)
  { // request confirmation
    CGUIDialogYesNo* pDialog = new CGUIDialogYesNo();
    if (pDialog->ShowAndGetInput(g_localizeStrings.Get(23009), pItem->GetProperty("Addon.Name"), "", g_localizeStrings.Get(23010)))
    {
      CAddon addon;
      if (g_addonmanager.GetAddonFromGUID(pItem->GetProperty("Addon.GUID"), addon))
      {
        g_addonmanager.GetCallbackForType(m_type)->RequestRemoval(&addon);
        g_addonmanager.DisableAddon(addon.UUID(), m_type);
        m_changed = true;
        Update();
        return true;
      }
    }
  }
  return false;
}

CFileItemPtr CGUIDialogAddonBrowser::GetCurrentListItem(int offset)
{
  return CFileItemPtr();
}

