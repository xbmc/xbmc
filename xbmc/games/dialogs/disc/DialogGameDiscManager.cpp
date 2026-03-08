/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscManager.h"

#include "ServiceBroker.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/disc/DiscManagerActions.h"
#include "games/dialogs/disc/DiscManagerButtons.h"
#include "games/dialogs/disc/DiscManagerDiscList.h"
#include "games/dialogs/disc/DiscManagerGame.h"
#include "games/dialogs/disc/DiscManagerMenu.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_DISC_MANAGER_MENU = 3;
constexpr int CONTROL_DISC_MANAGER_DISC_LIST = 108321;

constexpr auto PROPERTY_SHOW_MENU = "GameDiscManager.ShowMenu";
constexpr auto PROPERTY_SHOW_DISC_LIST = "GameDiscManager.ShowDiscList";
} // namespace

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml"),
    m_discActions(std::make_unique<CDiscManagerActions>(*this)),
    m_discButtons(std::make_unique<CDiscManagerButtons>(*this, *m_discActions)),
    m_discGame(std::make_unique<CDiscManagerGame>())
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameDiscManager::OnInitWindow()
{
  // Call ancestor
  CGUIDialog::OnInitWindow();

  // Get active game add-on
  m_gameClient = CDiscManagerGame::GetGameClient();

  // Initialize dialog
  InitializeDialog();

  // Update the menu with the initial state
  UpdateMenu();

  // Show the main menu
  ShowControl(CONTROL_DISC_MANAGER_MENU);
}

void CDialogGameDiscManager::InitializeDialog()
{
  // Initialize dialog components
  m_discActions->Initialize(m_gameClient);
  m_discGame->Initialize(m_gameClient);

  // If we're not playing a game, we're done here
  if (!m_gameClient)
    return;

  //
  // Initialize main menu
  //
  if (CGUIBaseContainer* discMgrMenu = GetMainMenu())
  {
    discMgrMenu->SetListProvider(
        std::make_unique<CDiscManagerMenu>(*m_discActions, *m_discGame, CONTROL_DISC_MANAGER_MENU));
  }
  else
  {
    CLog::Log(LOGERROR, "Missing main menu list with control ID {}", CONTROL_DISC_MANAGER_MENU);
  }

  //
  // Initialize disc selection list
  //
  if (CGUIBaseContainer* discMgrDiscList = GetDiscList())
  {
    discMgrDiscList->SetListProvider(std::make_unique<CDiscManagerDiscList>(
        m_gameClient, *this, CONTROL_DISC_MANAGER_DISC_LIST));
  }
  else
  {
    CLog::Log(LOGERROR, "Missing disc list with control ID {}", CONTROL_DISC_MANAGER_DISC_LIST);
  }
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  // Deinitialize dialogs components
  m_discActions->Deinitialize();
  m_discGame->Deinitialize();

  // Reset game add-on
  m_gameClient.reset();

  // Call ancestor
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CDialogGameDiscManager::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    {
      // Check if the disc list is visible
      if (GetProperty(PROPERTY_SHOW_DISC_LIST).asBoolean())
      {
        // Return to the main menu
        ShowControl(CONTROL_DISC_MANAGER_MENU);
        return true;
      }
      break;
    }
    default:
      break;
  }

  // Call ancestor
  return CGUIDialog::OnAction(action);
}

bool CDialogGameDiscManager::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (m_discButtons->OnClick(message.GetSenderId()))
        return true;
      break;
    }
    default:
      break;
  }

  // Call ancestor
  return CGUIDialog::OnMessage(message);
}

void CDialogGameDiscManager::UpdateMenu()
{
  CGUIBaseContainer* discMenu = GetMainMenu();
  if (discMenu != nullptr)
    discMenu->UpdateListProvider(true);

  bool ejected;
  std::string selectedDisc;
  m_discGame->GetState(ejected, selectedDisc);

  m_discButtons->UpdateButtons(ejected, selectedDisc);
}

void CDialogGameDiscManager::SelectDiscToInsert(std::optional<size_t> selectedIndex,
                                                std::function<void(std::optional<size_t>)> callback)
{
  m_insertCallback = callback;

  // Clear the disc list
  ClearDiscList();

  // Find the item index to focus/select
  const unsigned int selectedItemIndex =
      m_discGame->GetSelectedIndex(selectedIndex, AllowSelectNoDisc());

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Select the current disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST,
                            static_cast<int64_t>(selectedItemIndex));
  OnMessage(msgSelectDisc);
}

void CDialogGameDiscManager::SelectDiscToRemove(std::function<void(size_t)> callback)
{
  m_removeCallback = callback;

  // Clear the disc list
  ClearDiscList();

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Select the first disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST, 0);
  OnMessage(msgSelectDisc);
}

void CDialogGameDiscManager::OnDiscSelect(size_t discIndex, bool isNoDisc)
{
  if (m_insertCallback)
    m_insertCallback(isNoDisc ? std::nullopt : std::optional<size_t>{discIndex});
  else if (m_removeCallback && !isNoDisc)
    m_removeCallback(discIndex);

  // Return to the main menu
  ShowControl(CONTROL_DISC_MANAGER_MENU);
}

bool CDialogGameDiscManager::AllowSelectNoDisc() const
{
  if (m_insertCallback)
    return true;

  return false;
}

void CDialogGameDiscManager::ClearDiscList()
{
  CGUIBaseContainer* discMgrDiscList = GetDiscList();
  if (discMgrDiscList != nullptr)
  {
    discMgrDiscList->FreeResources(true);
    discMgrDiscList->AllocResources();
  }
}

void CDialogGameDiscManager::ShowControl(int controlId)
{
  if (controlId == CONTROL_DISC_MANAGER_MENU)
  {
    SetProperty(PROPERTY_SHOW_MENU, true);
    SetProperty(PROPERTY_SHOW_DISC_LIST, false);

    // Give focus to main menu
    FocusMainMenu();

    // If we're leaving the disc list, reset the callbacks
    m_insertCallback = {};
    m_removeCallback = {};
  }
  else if (controlId == CONTROL_DISC_MANAGER_DISC_LIST)
  {
    SetProperty(PROPERTY_SHOW_MENU, false);
    SetProperty(PROPERTY_SHOW_DISC_LIST, true);

    // Give focus to disc list
    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgSetFocus);
  }
}

void CDialogGameDiscManager::FocusMainMenu()
{
  if (GetMainMenu() != nullptr)
  {
    // Focus main menu
    CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_MENU, 0);
    OnMessage(msgFocus);

    // Select first "Select disc" item in the menu
    CGUIMessage msgSelectFirst(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_MENU, 0);
    OnMessage(msgSelectFirst);
  }
  else
  {
    m_discButtons->SetFocus();
  }
}

CGUIBaseContainer* CDialogGameDiscManager::GetMainMenu()
{
  return dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_MENU));
}

CGUIBaseContainer* CDialogGameDiscManager::GetDiscList()
{
  return dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
}
