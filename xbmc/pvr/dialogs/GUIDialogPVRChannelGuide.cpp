/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelGuide.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "view/ViewState.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/Epg.h"

using namespace PVR;

#define CONTROL_LIST  11

CGUIDialogPVRChannelGuide::CGUIDialogPVRChannelGuide()
    : CGUIDialog(WINDOW_DIALOG_PVR_CHANNEL_GUIDE, "DialogPVRChannelGuide.xml")
{
  m_vecItems.reset(new CFileItemList);
}

bool CGUIDialogPVRChannelGuide::OnAction(const CAction &action)
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
      return OnContextMenu(m_viewControl.GetSelectedItem());

    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogPVRChannelGuide::Open(const CPVRChannelPtr &channel)
{
  m_channel = channel;
  CGUIDialog::Open();
}

void CGUIDialogPVRChannelGuide::OnInitWindow()
{
  // no user-specific channel is set use current playing channel
  if (!m_channel)
    m_channel = CServiceBroker::GetPVRManager().GetPlayingChannel();

  // no channel at all, close the dialog
  if (!m_channel)
  {
    Close();
    return;
  }

  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  m_channel->GetEPG(*m_vecItems);
  m_viewControl.SetItems(*m_vecItems);

  // call init
  CGUIDialog::OnInitWindow();

  // select the active entry
  unsigned int iSelectedItem = 0;
  for (int iEpgPtr = 0; iEpgPtr < m_vecItems->Size(); ++iEpgPtr)
  {
    CFileItemPtr entry = m_vecItems->Get(iEpgPtr);
    if (entry->HasEPGInfoTag() && entry->GetEPGInfoTag()->IsActive())
    {
      iSelectedItem = iEpgPtr;
      break;
    }
  }
  m_viewControl.SetSelectedItem(iSelectedItem);
}

void CGUIDialogPVRChannelGuide::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  m_channel.reset();
  Clear();
}

void CGUIDialogPVRChannelGuide::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelGuide::ShowInfo(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;

  CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(m_vecItems->Get(item));
}

bool CGUIDialogPVRChannelGuide::OnContextMenu(int itemIdx)
{
  auto InRange = [](size_t i, std::pair<size_t, size_t> range){ return i >= range.first && i < range.second; };

  if (itemIdx < 0 || itemIdx >= m_vecItems->Size())
    return false;

  const CFileItemPtr item = m_vecItems->Get(itemIdx);
  if (!item)
    return false;

  CContextButtons buttons;

  // Add the global menu
  const ContextMenuView globalMenu = CServiceBroker::GetContextMenuManager().GetItems(*item, CContextMenuManager::MAIN);
  auto globalMenuRange = std::make_pair(buttons.size(), buttons.size() + globalMenu.size());
  for (const auto& menu : globalMenu)
    buttons.emplace_back(~buttons.size(), menu->GetLabel(*item));

  // Add addon menus
  const ContextMenuView addonMenu = CServiceBroker::GetContextMenuManager().GetAddonItems(*item, CContextMenuManager::MAIN);
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

void CGUIDialogPVRChannelGuide::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRChannelGuide::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogPVRChannelGuide::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}
