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
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;
using namespace RETRO;

CDialogInGameSaves::CDialogInGameSaves() : CDialogGameVideoSelect(WINDOW_DIALOG_IN_GAME_SAVES)
{
}

std::string CDialogInGameSaves::GetHeading()
{
  return g_localizeStrings.Get(35249); // "Saved games"
}

void CDialogInGameSaves::PreInit()
{
  m_items.Clear();

  InitSavedGames();

  CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(15314)); // "Save progress"
  item->SetArt("icon", "DefaultAddSource.png");
  item->SetPath("");
  item->SetProperty(SAVESTATE_CAPTION,
                    g_localizeStrings.Get(15315)); // "Save progress to new save file"

  m_items.AddFront(item, 0);
}

void CDialogInGameSaves::InitSavedGames()
{
  auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();

  // save current game
  gameSettings->CreateSavestate(true);

  CSavestateDatabase db;
  db.GetSavestatesNav(m_items, gameSettings->GetPlayingGame(), gameSettings->GameClientID());

  m_items.Sort(SortByDate, SortOrderDescending);
}

void CDialogInGameSaves::GetItems(CFileItemList& items)
{
  std::for_each(m_items.cbegin(), m_items.cend(), [&items](const auto& item) { items.Add(item); });
}

void CDialogInGameSaves::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
    m_focusedItemIndex = index;
}

unsigned int CDialogInGameSaves::GetFocusedItem() const
{
  return m_focusedControl;
}

void CDialogInGameSaves::PostExit()
{
  m_items.Clear();
}

void CDialogInGameSaves::OnClickAction()
{
  if (static_cast<int>(m_focusedItemIndex) < m_items.Size())
  {
    auto gameSettings = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    std::string savePath = m_items[m_focusedItemIndex]->GetPath();

    if (savePath.empty())
    {
      gameSettings->CreateSavestate(false);
    }
    else
      gameSettings->LoadSavestate(savePath);

    gameSettings->CloseOSD();
  }
}
