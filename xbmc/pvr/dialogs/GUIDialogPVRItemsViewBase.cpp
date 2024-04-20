/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRItemsViewBase.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "view/ViewState.h"

#include <utility>

#define CONTROL_LIST 11

using namespace PVR;

CGUIDialogPVRItemsViewBase::CGUIDialogPVRItemsViewBase(int id, const std::string& xmlFile)
  : CGUIDialog(id, xmlFile), m_vecItems(new CFileItemList)
{
}

void CGUIDialogPVRItemsViewBase::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRItemsViewBase::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIDialogPVRItemsViewBase::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
}

void CGUIDialogPVRItemsViewBase::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  Clear();
}

bool CGUIDialogPVRItemsViewBase::OnAction(const CAction& action)
{
  if (m_viewControl.HasControl(GetFocusedControlID()))
  {
    switch (action.GetID())
    {
      case ACTION_SHOW_INFO:
      case ACTION_SELECT_ITEM:
      case ACTION_MOUSE_LEFT_CLICK:
        ShowInfo(m_viewControl.GetSelectedItem());
        return true;

      case ACTION_CONTEXT_MENU:
      case ACTION_MOUSE_RIGHT_CLICK:
        return ContextMenu(m_viewControl.GetSelectedItem());

      default:
        break;
    }
  }
  return CGUIDialog::OnAction(action);
}

CGUIControl* CGUIDialogPVRItemsViewBase::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIDialog::GetFirstFocusableControl(id);
}

void CGUIDialogPVRItemsViewBase::ShowInfo(int itemIdx)
{
  if (itemIdx < 0 || itemIdx >= m_vecItems->Size())
    return;

  const std::shared_ptr<CFileItem> item = m_vecItems->Get(itemIdx);
  if (!item)
    return;

  CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(*item);
}

bool CGUIDialogPVRItemsViewBase::ContextMenu(int itemIdx)
{
  auto InRange = [](size_t i, std::pair<size_t, size_t> range) {
    return i >= range.first && i < range.second;
  };

  if (itemIdx < 0 || itemIdx >= m_vecItems->Size())
    return false;

  const CFileItemPtr item = m_vecItems->Get(itemIdx);
  if (!item)
    return false;

  CContextButtons buttons;

  // Add the global menu
  const ContextMenuView globalMenu =
      CServiceBroker::GetContextMenuManager().GetItems(*item, CContextMenuManager::MAIN);
  auto globalMenuRange = std::make_pair(buttons.size(), buttons.size() + globalMenu.size());
  for (const auto& menu : globalMenu)
    buttons.emplace_back(~buttons.size(), menu->GetLabel(*item));

  // Add addon menus
  const ContextMenuView addonMenu =
      CServiceBroker::GetContextMenuManager().GetAddonItems(*item, CContextMenuManager::MAIN);
  auto addonMenuRange = std::make_pair(buttons.size(), buttons.size() + addonMenu.size());
  for (const auto& menu : addonMenu)
    buttons.emplace_back(~buttons.size(), menu->GetLabel(*item));

  if (buttons.empty())
    return true;

  int idx = CGUIDialogContextMenu::Show(buttons);
  if (idx < 0 || idx >= static_cast<int>(buttons.size()))
    return false;

  Close();

  if (InRange(idx, globalMenuRange))
    return CONTEXTMENU::LoopFrom(*globalMenu[idx - globalMenuRange.first], item);

  return CONTEXTMENU::LoopFrom(*addonMenu[idx - addonMenuRange.first], item);
}

void CGUIDialogPVRItemsViewBase::Init()
{
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  Clear();
}

void CGUIDialogPVRItemsViewBase::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}
