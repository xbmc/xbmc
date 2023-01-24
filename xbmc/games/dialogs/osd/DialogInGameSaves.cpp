/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogInGameSaves.h"

#include "ServiceBroker.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
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
  if (!savestatePath.empty())
  {
    std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
    RETRO::CSavestateDatabase db;
    if (db.GetSavestate(savestatePath, *savestate))
    {
      CFileItemPtr item = std::make_unique<CFileItem>();
      RETRO::CSavestateDatabase::GetSavestateItem(*savestate, savestatePath, *item);
      item->SetPath(savestatePath);

      m_savestateItems.AddFront(std::move(item), 0);

      RefreshList();
    }
  }
  else
  {
    // "Error"
    // "An unknown error has occurred."
    CGUIDialogOK::ShowAndGetInput(257, 24071);
  }
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
    std::unique_ptr<RETRO::ISavestate> savestate = RETRO::CSavestateDatabase::AllocateSavestate();
    RETRO::CSavestateDatabase db;
    if (db.GetSavestate(savestatePath, *savestate))
    {
      RETRO::CSavestateDatabase::GetSavestateItem(*savestate, savestatePath, focusedItem);
      m_savestateItems.Sort(SortByDate, SortOrderDescending);

      RefreshList();
    }
  }
  else
  {
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
    }
    else
    {
      // "Error"
      // "An unknown error has occurred."
      CGUIDialogOK::ShowAndGetInput(257, 24071);
    }
  }
}
