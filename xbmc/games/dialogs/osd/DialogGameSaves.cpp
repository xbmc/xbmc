/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameSaves.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "utils/FileUtils.h"
#include "utils/Variant.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace GAME;

CDialogGameSaves::CDialogGameSaves()
  : CGUIDialog(WINDOW_DIALOG_GAME_SAVES, "DialogSelect.xml"),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecList(std::make_unique<CFileItemList>())
{
}

bool CDialogGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int actionId = message.GetParam1();

      switch (actionId)
      {
        case ACTION_SELECT_ITEM:
        case ACTION_MOUSE_LEFT_CLICK:
        {
          int selectedId = m_viewControl->GetSelectedItem();
          if (0 <= selectedId && selectedId < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedId);
            if (item)
            {
              for (int i = 0; i < m_vecList->Size(); i++)
                m_vecList->Get(i)->Select(false);

              item->Select(true);

              OnSelect(*item);

              return true;
            }
          }
          break;
        }
        case ACTION_CONTEXT_MENU:
        case ACTION_MOUSE_RIGHT_CLICK:
        {
          int selectedItem = m_viewControl->GetSelectedItem();
          if (selectedItem >= 0 && selectedItem < m_vecList->Size())
          {
            CFileItemPtr item = m_vecList->Get(selectedItem);
            if (item)
            {
              OnContextMenu(*item);
              return true;
            }
          }
          break;
        }
        case ACTION_RENAME_ITEM:
        {
          const int controlId = message.GetSenderId();
          if (m_viewControl->HasControl(controlId))
          {
            int selectedItem = m_viewControl->GetSelectedItem();
            if (selectedItem >= 0 && selectedItem < m_vecList->Size())
            {
              CFileItemPtr item = m_vecList->Get(selectedItem);
              if (item)
              {
                OnRename(*item);
                return true;
              }
            }
          }
          break;
        }
        case ACTION_DELETE_ITEM:
        {
          const int controlId = message.GetSenderId();
          if (m_viewControl->HasControl(controlId))
          {
            int selectedItem = m_viewControl->GetSelectedItem();
            if (selectedItem >= 0 && selectedItem < m_vecList->Size())
            {
              CFileItemPtr item = m_vecList->Get(selectedItem);
              if (item)
              {
                OnDelete(*item);
                return true;
              }
            }
          }
          break;
        }
        default:
          break;
      }

      const int controlId = message.GetSenderId();
      switch (controlId)
      {
        case CONTROL_SAVES_NEW_BUTTON:
        {
          m_bNewPressed = true;
          Close();
          break;
        }
        case CONTROL_SAVES_CANCEL_BUTTON:
        {
          m_selectedItem.reset();
          m_vecList->Clear();
          m_bConfirmed = false;
          Close();
          break;
        }
        default:
          break;
      }

      break;
    }

    case GUI_MSG_SETFOCUS:
    {
      const int controlId = message.GetControlId();
      if (m_viewControl->HasControl(controlId))
      {
        if (m_vecList->IsEmpty())
        {
          SET_CONTROL_FOCUS(CONTROL_SAVES_NEW_BUTTON, 0);
          return true;
        }

        if (m_viewControl->GetCurrentControl() != controlId)
        {
          m_viewControl->SetFocused();
          return true;
        }
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameSaves::FrameMove()
{
  CGUIControl* itemContainer = GetControl(CONTROL_SAVES_DETAILED_LIST);
  if (itemContainer != nullptr)
  {
    if (itemContainer->HasFocus())
    {
      int selectedItem = m_viewControl->GetSelectedItem();
      if (selectedItem >= 0 && selectedItem < m_vecList->Size())
      {
        CFileItemPtr item = m_vecList->Get(selectedItem);
        if (item)
          OnFocus(*item);
      }
    }
    else
    {
      OnFocusLost();
    }
  }

  CGUIDialog::FrameMove();
}

void CDialogGameSaves::OnInitWindow()
{
  m_viewControl->SetItems(*m_vecList);
  m_viewControl->SetCurrentView(CONTROL_SAVES_DETAILED_LIST);

  CGUIDialog::OnInitWindow();

  // Select the first item
  m_viewControl->SetSelectedItem(0);

  // There's a race condition where the item's focus sends the update message
  // before the window is fully initialized, so explicitly set the info now.
  if (!m_vecList->IsEmpty())
  {
    CFileItemPtr item = m_vecList->Get(0);
    if (item)
    {
      const std::string gameClientId = item->GetProperty(SAVESTATE_GAME_CLIENT).asString();
      if (!gameClientId.empty())
      {
        std::string emulatorName;
        std::string emulatorIcon;

        using namespace ADDON;

        AddonPtr addon;
        CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
        if (addonManager.GetAddon(m_currentGameClient, addon, OnlyEnabled::CHOICE_NO))
        {
          std::shared_ptr<CGameClient> gameClient = std::dynamic_pointer_cast<CGameClient>(addon);
          if (gameClient)
          {
            m_currentGameClient = gameClient->ID();

            emulatorName = gameClient->GetEmulatorName();
            emulatorIcon = gameClient->Icon();
          }
        }

        if (!emulatorName.empty())
        {
          CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_EMULATOR_NAME);
          message.SetLabel(emulatorName);
          OnMessage(message);
        }
        if (!emulatorIcon.empty())
        {
          CGUIMessage message(GUI_MSG_SET_FILENAME, GetID(), CONTROL_SAVES_EMULATOR_ICON);
          message.SetLabel(emulatorIcon);
          OnMessage(message);
        }
      }

      const std::string caption = item->GetProperty(SAVESTATE_CAPTION).asString();
      if (!caption.empty())
      {
        m_currentCaption = caption;

        CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_DESCRIPTION);
        message.SetLabel(m_currentCaption);
        OnMessage(message);
      }
    }
  }
}

void CDialogGameSaves::OnDeinitWindow(int nextWindowID)
{
  m_viewControl->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);

  // Get selected item
  for (int i = 0; i < m_vecList->Size(); ++i)
  {
    CFileItemPtr item = m_vecList->Get(i);
    if (item->IsSelected())
    {
      m_selectedItem = item;
      break;
    }
  }

  m_vecList->Clear();
}

void CDialogGameSaves::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_SAVES_DETAILED_LIST));
}

void CDialogGameSaves::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl->Reset();
}

void CDialogGameSaves::Reset()
{
  m_bConfirmed = false;
  m_bNewPressed = false;

  m_vecList->Clear();
  m_selectedItem.reset();
}

bool CDialogGameSaves::Open(const std::string& gamePath)
{
  CFileItemList items;

  RETRO::CSavestateDatabase db;
  if (!db.GetSavestatesNav(items, gamePath))
    return false;

  if (items.IsEmpty())
    return false;

  items.Sort(SortByDate, SortOrderDescending);

  SetItems(items);

  CGUIDialog::Open();

  return true;
}

std::string CDialogGameSaves::GetSelectedItemPath()
{
  if (m_selectedItem)
    return m_selectedItem->GetPath();

  return "";
}

void CDialogGameSaves::SetItems(const CFileItemList& itemList)
{
  m_vecList->Clear();

  // Need to make internal copy of list to be sure dialog is owner of it
  m_vecList->Copy(itemList);

  m_viewControl->SetItems(*m_vecList);
}

void CDialogGameSaves::OnSelect(const CFileItem& item)
{
  m_bConfirmed = true;
  Close();
}

void CDialogGameSaves::OnFocus(const CFileItem& item)
{
  const std::string caption = item.GetProperty(SAVESTATE_CAPTION).asString();
  const std::string gameClientId = item.GetProperty(SAVESTATE_GAME_CLIENT).asString();

  HandleCaption(caption);
  HandleGameClient(gameClientId);
}

void CDialogGameSaves::OnFocusLost()
{
  HandleCaption("");
  HandleGameClient("");
}

void CDialogGameSaves::OnContextMenu(CFileItem& item)
{
  CContextButtons buttons;

  buttons.Add(0, 118); // "Rename"
  buttons.Add(1, 117); // "Delete"

  const int index = CGUIDialogContextMenu::Show(buttons);

  if (index == 0)
    OnRename(item);
  else if (index == 1)
    OnDelete(item);
}

void CDialogGameSaves::OnRename(CFileItem& item)
{
  const std::string& savestatePath = item.GetPath();

  // Get savestate properties
  RETRO::CSavestateDatabase db;
  std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
  db.GetSavestate(savestatePath, *savestate);

  std::string label(savestate->Label());

  // "Enter new filename"
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16013)}, true) &&
      label != savestate->Label())
  {
    std::unique_ptr<RETRO::ISavestate> newSavestate = db.RenameSavestate(savestatePath, label);
    if (newSavestate)
    {
      RETRO::CSavestateDatabase::GetSavestateItem(*newSavestate, savestatePath, item);

      // Refresh thumbnails
      m_viewControl->SetItems(*m_vecList);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::OnDelete(CFileItem& item)
{
  // "Confirm delete"
  // "Would you like to delete the selected file(s)?[CR]Warning - this action can't be undone!"
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
  {
    const std::string& savestatePath = item.GetPath();

    RETRO::CSavestateDatabase db;
    if (db.DeleteSavestate(savestatePath))
    {
      m_vecList->Remove(&item);

      // Refresh thumbnails
      m_viewControl->SetItems(*m_vecList);
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogGameSaves::HandleCaption(const std::string& caption)
{
  if (caption != m_currentCaption)
  {
    m_currentCaption = caption;

    // Update the GUI label
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_DESCRIPTION);
    msg.SetLabel(m_currentCaption);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}

void CDialogGameSaves::HandleGameClient(const std::string& gameClientId)
{
  if (gameClientId == m_currentGameClient)
    return;

  m_currentGameClient = gameClientId;
  if (m_currentGameClient.empty())
    return;

  // Get game client properties
  std::shared_ptr<CGameClient> gameClient;
  std::string emulatorName;
  std::string iconPath;

  using namespace ADDON;

  AddonPtr addon;
  CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
  if (addonManager.GetAddon(m_currentGameClient, addon, OnlyEnabled::CHOICE_NO))
    gameClient = std::dynamic_pointer_cast<CGameClient>(addon);

  if (gameClient)
  {
    emulatorName = gameClient->GetEmulatorName();
    iconPath = gameClient->Icon();
  }

  // Update the GUI elements
  if (!emulatorName.empty())
  {
    CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), CONTROL_SAVES_EMULATOR_NAME);
    message.SetLabel(emulatorName);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, GetID());
  }
  if (!iconPath.empty())
  {
    CGUIMessage message(GUI_MSG_SET_FILENAME, GetID(), CONTROL_SAVES_EMULATOR_ICON);
    message.SetLabel(iconPath);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, GetID());
  }
}
