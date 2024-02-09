/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogInGameSaves.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "XBDateTime.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "cores/RetroPlayer/guicontrols/GUIGameControl.h"
#include "cores/RetroPlayer/playback/IPlayback.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;
using namespace RETRO;

namespace
{
CFileItemPtr CreateNewSaveItem()
{
  CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(15314)); // "Save"

  // A nonexistent path ensures a gamewindow control won't render any pixels
  item->SetPath(NO_PIXEL_DATA);
  item->SetArt("icon", "DefaultAddSource.png");
  item->SetProperty(SAVESTATE_CAPTION,
                    g_localizeStrings.Get(15315)); // "Save progress to a new save file"

  return item;
}
} // namespace

CDialogInGameSaves::CDialogInGameSaves()
  : CDialogGameVideoSelect(WINDOW_DIALOG_IN_GAME_SAVES), m_newSaveItem(CreateNewSaveItem())
{
}

bool CDialogInGameSaves::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_REFRESH_THUMBS:
    {
      if (message.GetControlId() == GetID())
      {
        const std::string& itemPath = message.GetStringParam();
        std::shared_ptr<CGUIListItem> itemInfo = message.GetItem();

        if (!itemPath.empty())
        {
          OnItemRefresh(itemPath, itemInfo);
        }
        else
        {
          InitSavedGames();
          RefreshList();
        }

        return true;
      }
      break;
    }
    default:
      break;
  }

  return CDialogGameVideoSelect::OnMessage(message);
}

std::string CDialogInGameSaves::GetHeading()
{
  return g_localizeStrings.Get(35249); // "Save / Load"
}

void CDialogInGameSaves::PreInit()
{
  InitSavedGames();
}

void CDialogInGameSaves::InitSavedGames()
{
  m_savestateItems.Clear();

  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  CSavestateDatabase db;
  db.GetSavestatesNav(m_savestateItems, gameSettings->GetPlayingGame(),
                      gameSettings->GameClientID());

  m_savestateItems.Sort(SortByDate, SortOrderDescending);
}

void CDialogInGameSaves::GetItems(CFileItemList& items)
{
  items.Add(m_newSaveItem);
  std::for_each(m_savestateItems.cbegin(), m_savestateItems.cend(),
                [&items](const auto& item) { items.Add(item); });
}

void CDialogInGameSaves::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < 1 + m_savestateItems.Size())
    m_focusedItemIndex = index;
}

unsigned int CDialogInGameSaves::GetFocusedItem() const
{
  return m_focusedControl;
}

void CDialogInGameSaves::OnItemRefresh(const std::string& itemPath,
                                       const std::shared_ptr<CGUIListItem>& itemInfo)
{
  // Turn the message params into a savestate item
  CFileItemPtr item = TranslateMessageItem(itemPath, itemInfo);
  if (item)
  {
    // Look up existing savestate by path
    auto it =
        std::find_if(m_savestateItems.cbegin(), m_savestateItems.cend(),
                     [&itemPath](const CFileItemPtr& item) { return item->GetPath() == itemPath; });

    // Update savestate or add a new one
    if (it != m_savestateItems.cend())
      **it = *item;
    else
      m_savestateItems.AddFront(item, 0);

    RefreshList();
  }
}

void CDialogInGameSaves::PostExit()
{
  m_savestateItems.Clear();
}

bool CDialogInGameSaves::OnClickAction()
{
  if (static_cast<int>(m_focusedItemIndex) < 1 + m_savestateItems.Size())
  {
    if (m_focusedItemIndex <= 0)
    {
      OnNewSave();
      return true;
    }
    else
    {
      CFileItemPtr focusedItem = m_savestateItems[m_focusedItemIndex - 1];
      if (focusedItem)
      {
        OnLoad(*focusedItem);
        return true;
      }
    }
  }

  return false;
}

bool CDialogInGameSaves::OnMenuAction()
{
  // Start at index 1 to account for leading "Save" item
  if (1 <= m_focusedItemIndex && static_cast<int>(m_focusedItemIndex) < 1 + m_savestateItems.Size())
  {
    CFileItemPtr focusedItem = m_savestateItems[m_focusedItemIndex - 1];
    if (focusedItem)
    {
      CContextButtons buttons;

      buttons.Add(0, 13206); // "Overwrite"
      buttons.Add(1, 118); // "Rename"
      buttons.Add(2, 117); // "Delete"

      const int index = CGUIDialogContextMenu::Show(buttons);

      if (index == 0)
        OnOverwrite(*focusedItem);
      if (index == 1)
        OnRename(*focusedItem);
      else if (index == 2)
        OnDelete(*focusedItem);

      return true;
    }
  }

  return false;
}

bool CDialogInGameSaves::OnOverwriteAction()
{
  // Start at index 1 to account for leading "Save" item
  if (1 <= m_focusedItemIndex && static_cast<int>(m_focusedItemIndex) < 1 + m_savestateItems.Size())
  {
    CFileItemPtr focusedItem = m_savestateItems[m_focusedItemIndex - 1];
    if (focusedItem)
    {
      OnOverwrite(*focusedItem);
      return true;
    }
  }

  return false;
}

bool CDialogInGameSaves::OnRenameAction()
{
  // Start at index 1 to account for leading "Save" item
  if (1 <= m_focusedItemIndex && static_cast<int>(m_focusedItemIndex) < 1 + m_savestateItems.Size())
  {
    CFileItemPtr focusedItem = m_savestateItems[m_focusedItemIndex - 1];
    if (focusedItem)
    {
      OnRename(*focusedItem);
      return true;
    }
  }

  return false;
}

bool CDialogInGameSaves::OnDeleteAction()
{
  // Start at index 1 to account for leading "Save" item
  if (1 <= m_focusedItemIndex && static_cast<int>(m_focusedItemIndex) < 1 + m_savestateItems.Size())
  {
    CFileItemPtr focusedItem = m_savestateItems[m_focusedItemIndex - 1];
    if (focusedItem)
    {
      OnDelete(*focusedItem);
      return true;
    }
  }

  return false;
}

void CDialogInGameSaves::OnNewSave()
{
  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  const std::string savestatePath = gameSettings->CreateSavestate(false);
  if (savestatePath.empty())
  {
    // "Error"
    // "An unknown error has occurred."
    CGUIDialogOK::ShowAndGetInput(257, 24071);
    return;
  }

  // Create a simulated savestate to update the GUI faster. We will be notified
  // of the real savestate info via OnMessage() when the savestate creation
  // completes.
  auto savestate = RETRO::CSavestateDatabase::AllocateSavestate();

  savestate->SetType(SAVE_TYPE::MANUAL);
  savestate->SetCreated(CDateTime::GetUTCDateTime());

  savestate->Finalize();

  CFileItemPtr item = std::make_shared<CFileItem>();
  CSavestateDatabase::GetSavestateItem(*savestate, savestatePath, *item);

  m_savestateItems.AddFront(item, 0);

  RefreshList();
}

void CDialogInGameSaves::OnLoad(CFileItem& focusedItem)
{
  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  // Load savestate
  if (gameSettings->LoadSavestate(focusedItem.GetPath()))
  {
    // Close OSD on successful load
    gameSettings->CloseOSD();
  }
  else
  {
    // "Error"
    // "An unknown error has occurred."
    CGUIDialogOK::ShowAndGetInput(257, 24071);
  }
}

void CDialogInGameSaves::OnOverwrite(CFileItem& focusedItem)
{
  std::string savestatePath = focusedItem.GetPath();
  if (savestatePath.empty())
    return;

  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  // Update savestate
  if (gameSettings->UpdateSavestate(savestatePath))
  {
    // Create a simulated savestate to update the GUI faster. We will be
    // notified of the real savestate info via OnMessage() when the
    // overwriting completes.
    auto savestate = RETRO::CSavestateDatabase::AllocateSavestate();

    savestate->SetType(SAVE_TYPE::MANUAL);
    savestate->SetLabel(focusedItem.GetProperty(SAVESTATE_LABEL).asString());
    savestate->SetCaption(focusedItem.GetProperty(SAVESTATE_CAPTION).asString());
    savestate->SetCreated(CDateTime::GetUTCDateTime());

    savestate->Finalize();

    CSavestateDatabase::GetSavestateItem(*savestate, savestatePath, focusedItem);

    RefreshList();
  }
  else
  {
    // Log an error and notify the user
    CLog::Log(LOGERROR, "Failed to overwrite savestate at {}", CURL::GetRedacted(savestatePath));

    // "Error"
    // "An unknown error has occurred."
    CGUIDialogOK::ShowAndGetInput(257, 24071);
  }
}

void CDialogInGameSaves::OnRename(CFileItem& focusedItem)
{
  const std::string& savestatePath = focusedItem.GetPath();
  if (savestatePath.empty())
    return;

  RETRO::CSavestateDatabase db;

  std::string label;

  std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
  if (db.GetSavestate(savestatePath, *savestate))
    label = savestate->Label();

  // "Enter new filename"
  if (CGUIKeyboardFactory::ShowAndGetInput(label, CVariant{g_localizeStrings.Get(16013)}, true) &&
      label != savestate->Label())
  {
    std::unique_ptr<RETRO::ISavestate> newSavestate = db.RenameSavestate(savestatePath, label);
    if (newSavestate)
    {
      RETRO::CSavestateDatabase::GetSavestateItem(*newSavestate, savestatePath, focusedItem);

      RefreshList();
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

void CDialogInGameSaves::OnDelete(CFileItem& focusedItem)
{
  // "Confirm delete"
  // "Would you like to delete the selected file(s)?[CR]Warning - this action can't be undone!"
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, CVariant{125}))
  {
    RETRO::CSavestateDatabase db;
    if (db.DeleteSavestate(focusedItem.GetPath()))
    {
      m_savestateItems.Remove(&focusedItem);

      RefreshList();

      auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
      gameSettings->FreeSavestateResources(focusedItem.GetPath());
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}

CFileItemPtr CDialogInGameSaves::TranslateMessageItem(
    const std::string& messagePath, const std::shared_ptr<CGUIListItem>& messageItem)
{
  CFileItemPtr item;

  if (messageItem && messageItem->IsFileItem())
    item = std::static_pointer_cast<CFileItem>(messageItem);
  else if (messageItem)
    item = std::make_shared<CFileItem>(*messageItem);
  else if (!messagePath.empty())
  {
    item = std::make_shared<CFileItem>();

    // Load savestate if no item info was given
    auto savestate = RETRO::CSavestateDatabase::AllocateSavestate();
    RETRO::CSavestateDatabase db;
    if (db.GetSavestate(messagePath, *savestate))
      RETRO::CSavestateDatabase::GetSavestateItem(*savestate, messagePath, *item);
    else
      item.reset();
  }

  return item;
}
