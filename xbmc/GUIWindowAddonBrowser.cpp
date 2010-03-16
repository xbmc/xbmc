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
: CGUIWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.GetID() == ACTION_CONTEXT_MENU || action.GetID() == ACTION_MOUSE_RIGHT_CLICK)
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
          return OnContextMenu(iItem);
        }
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < CONTROL_START_BUTTONS+(int)m_categories.size())
      {
        m_currentCategory = focusedControl-CONTROL_START_BUTTONS;
        Update();
      }
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowAddonBrowser::ClearListItems()
{
  m_vecItems->Clear();
}

void CGUIWindowAddonBrowser::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  m_currentCategory = 0;
  SetupControls();
  Update();
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

  if (m_categories.empty())
    return;

  VECADDONS addons;
  CAddonMgr::Get()->GetAddons(m_categories[m_currentCategory], addons, CONTENT_NONE, false);

  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    CFileItemPtr pItem(new CFileItem(addon->ID(), false));
    pItem->SetLabel(addon->Name());
    pItem->SetProperty("Addon.ID", addon->ID());
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
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_currentCategory);
  ((CGUIButtonControl *)control)->SetSelected(true);

  if (selected != 0)
    SelectItem(selected);
}

void CGUIWindowAddonBrowser::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return ;
  CFileItemPtr pItem = (*m_vecItems)[iItem];
  CStdString strPath = pItem->m_strPath;

  // check if user is allowed to open this window
  if (g_settings.GetCurrentProfile().addonmanagerLocked() && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  AddonPtr addon;
  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  if (CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), addon, type, false))
  {
    if (addon->Disabled())
      CAddonMgr::Get()->EnableAddon(addon);
    else
      CAddonMgr::Get()->DisableAddon(addon);
    Update();
  }
}

bool CGUIWindowAddonBrowser::OnContextMenu(int iItem)
{
  // check if user is allowed to open this window
  if (g_settings.GetCurrentProfile().addonmanagerLocked() && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

  CGUIDialogContextMenu* pMenu = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu)
    return false;

  pMenu->Initialize();
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), addon, type, false))
    return false;

  int iSettingsLabel = 24020;
  int iDisableLabel = 24021;
  int iEnableLabel = 24022;

  int btn_Disable = -1;
  int btn_Enable = -1;
  int btn_Settings = -1;

  if (addon->Disabled())
    btn_Enable = pMenu->AddButton(iEnableLabel);
  else
  {
    btn_Disable = pMenu->AddButton(iDisableLabel);
    if (addon->HasSettings())
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

void CGUIWindowAddonBrowser::SetupControls()
{
  FreeControls();
  CGUIButtonControl* pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!pOriginalCategoryButton || !group)
    return;
  for (unsigned int i=ADDON::ADDON_UNKNOWN+1;i<ADDON_VIZ_LIBRARY;++i)
  {
    if (!CAddonMgr::Get()->HasAddons((ADDON::TYPE)i))
      continue;
    CGUIButtonControl* pButton = pOriginalCategoryButton->Clone();
    pButton->SetLabel(ADDON::TranslateType((ADDON::TYPE)i,true));
    pButton->SetID(CONTROL_START_BUTTONS+m_categories.size());
    pButton->SetVisible(true);
    pButton->AllocResources();
    group->AddControl(pButton);
    m_categories.push_back((ADDON::TYPE)i);
  }
}

void CGUIWindowAddonBrowser::FreeControls()
{
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_categories.clear();
}

