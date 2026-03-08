/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscManager.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/disc/DiscManagerActions.h"
#include "games/dialogs/disc/DiscManagerButtons.h"
#include "games/dialogs/disc/DiscManagerDiscList.h"
#include "games/dialogs/disc/DiscManagerGame.h"
#include "games/dialogs/disc/DiscManagerIDs.h"
#include "games/dialogs/disc/DiscManagerMenu.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
#include "guilib/WindowIDs.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

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

  // Focus the first item
  FocusMainMenuItem(MENU_INDEX_SELECT_DISC);
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
    CLog::Log(LOGDEBUG, "Missing main menu list with control ID {}", CONTROL_DISC_MANAGER_MENU);
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
      if (GetProperty(std::string{WINDOW_PROPERTY_SHOW_DISC_LIST}).asBoolean())
      {
        // Select the appropriate item based on whether we're selecting or
        // deleting a disc
        if (m_deleteCallback)
          FocusMainMenuItem(MENU_INDEX_DELETE_DISC);
        else
          FocusMainMenuItem(MENU_INDEX_SELECT_DISC);

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

void CDialogGameDiscManager::FocusMainMenuItem(unsigned int itemIndex)
{
  // Sanitize parameters
  if (itemIndex >= MENU_ITEM_COUNT)
    itemIndex = MENU_INDEX_SELECT_DISC;

  // Activate the main menu state
  ShowControl(CONTROL_DISC_MANAGER_MENU);

  if (GetMainMenu() != nullptr)
  {
    // Select the appropriate list item
    CONTROL_SELECT_ITEM(CONTROL_DISC_MANAGER_MENU, itemIndex);
  }
  else
  {
    // Focus the appropriate button
    m_discButtons->SetFocus(itemIndex);
  }
}

void CDialogGameDiscManager::SelectDiscToInsert(std::optional<size_t> selectedIndex,
                                                std::function<void(std::optional<size_t>)> callback)
{
  m_insertCallback = callback;

  const std::optional<std::string> selectedDiscPath = m_discGame->GetDiscPathByIndex(selectedIndex);

  // Clear the disc list
  ClearDiscList();

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Find the item index to focus/select
  unsigned int selectedItemIndex = m_discGame->GetSelectedIndex(selectedIndex, AllowSelectNoDisc());

  if (selectedDiscPath.has_value())
  {
    if (const CGUIBaseContainer* discList = GetDiscList(); discList != nullptr)
    {
      for (int i = 0;; ++i)
      {
        auto listItem = discList->GetListItem(i, INFOFLAG_LISTITEM_ABSOLUTE);
        if (!listItem)
          break;

        // The visible row is resolved after display-only sorting, so compare
        // normalized paths instead of raw strings when matching the previously
        // selected disc.
        if (const auto fileItem = std::dynamic_pointer_cast<CFileItem>(listItem);
            fileItem && URIUtils::PathEquals(fileItem->GetPath(), *selectedDiscPath))
        {
          selectedItemIndex = static_cast<unsigned int>(i);
          break;
        }
      }
    }
  }

  // Select the current disc
  CONTROL_SELECT_ITEM(CONTROL_DISC_MANAGER_DISC_LIST, selectedItemIndex);
}

void CDialogGameDiscManager::SelectDiscToDelete(std::function<void(size_t)> callback)
{
  m_deleteCallback = callback;

  // Clear the disc list
  ClearDiscList();

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Select the first disc
  CONTROL_SELECT_ITEM(CONTROL_DISC_MANAGER_DISC_LIST, 0);
}

void CDialogGameDiscManager::OnDiscSelect(size_t discIndex, bool isNoDisc)
{
  if (m_insertCallback)
    m_insertCallback(isNoDisc ? std::nullopt : std::optional<size_t>{discIndex});
  else if (m_deleteCallback && !isNoDisc)
    m_deleteCallback(discIndex);
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
    SetProperty(std::string{WINDOW_PROPERTY_SHOW_MENU}, true);
    SetProperty(std::string{WINDOW_PROPERTY_SHOW_DISC_LIST}, false);

    // Give focus to main menu, if present
    SET_CONTROL_FOCUS(CONTROL_DISC_MANAGER_MENU, 0);

    // If we're leaving the disc list, reset the callbacks
    m_insertCallback = {};
    m_deleteCallback = {};
  }
  else if (controlId == CONTROL_DISC_MANAGER_DISC_LIST)
  {
    SetProperty(std::string{WINDOW_PROPERTY_SHOW_MENU}, false);
    SetProperty(std::string{WINDOW_PROPERTY_SHOW_DISC_LIST}, true);

    // Give focus to disc list
    SET_CONTROL_FOCUS(CONTROL_DISC_MANAGER_DISC_LIST, 0);
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
