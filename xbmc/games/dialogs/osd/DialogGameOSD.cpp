/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameOSD.h"

#include "DialogGameOSDHelp.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/agents/input/AgentInput.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "peripherals/devices/Peripheral.h"

#include <chrono>
#include <string>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_MENU = 1103;
constexpr int CONTROL_PLAYER_LIST = 1200;
constexpr int CONTROL_BACKGROUND = 2001;

constexpr auto PLAYER_REFRESH_INTERVAL = std::chrono::seconds(2);

std::string BatteryStateToString(PERIPHERALS::PeripheralBatteryState state)
{
  switch (state)
  {
    case PERIPHERALS::PeripheralBatteryState::DISCHARGING:
      return "discharging";
    case PERIPHERALS::PeripheralBatteryState::CHARGING:
      return "charging";
    case PERIPHERALS::PeripheralBatteryState::FULL:
      return "full";
    default:
      break;
  }

  return "";
}
} // namespace

CDialogGameOSD::CDialogGameOSD()
  : CGUIDialog(WINDOW_DIALOG_GAME_OSD, "GameOSD.xml"),
    m_helpDialog(std::make_unique<CDialogGameOSDHelp>(*this)),
    m_playerItems(std::make_unique<CFileItemList>())
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameOSD::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    case ACTION_SHOW_OSD:
    case ACTION_PLAYER_PLAY:
    {
      // Disable OSD help if visible
      if (m_helpDialog->IsVisible() && CServiceBroker::IsServiceManagerUp())
      {
        GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
        if (gameSettings.ShowOSDHelp())
        {
          gameSettings.SetShowOSDHelp(false);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CDialogGameOSD::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  const auto* menu = dynamic_cast<CGUIControlGroupList*>(GetControl(CONTROL_MENU));
  auto* background = dynamic_cast<CGUIImage*>(GetControl(CONTROL_BACKGROUND));
  if (menu != nullptr && background != nullptr)
  {
    const auto* parentControl = background->GetParentControl();
    if (parentControl != nullptr)
    {
      // Match the cached menu height used by its parent grouplist for centering.
      const float height = parentControl->GetHeight() + menu->GetHeight();
      if (background->GetHeight() != height)
        background->SetHeight(height);
    }
  }

  if (m_playerTimeout.IsTimePast())
    UpdatePlayers();

  CGUIDialog::Process(currentTime, dirtyregions);
}

void CDialogGameOSD::OnInitWindow()
{
  // Init parent class
  CGUIDialog::OnInitWindow();

  // Init help dialog
  m_helpDialog->OnInitWindow();

  m_players.clear();
  UpdatePlayers();
}

void CDialogGameOSD::OnDeinitWindow(int nextWindowID)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PLAYER_LIST);
  OnMessage(msg);
  m_playerItems->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);

  if (CServiceBroker::IsServiceManagerUp())
  {
    GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
    gameSettings.SetShowOSDHelp(false);
  }
}

bool CDialogGameOSD::PlayInBackground(int dialogId)
{
  return dialogId == WINDOW_DIALOG_GAME_VOLUME;
}

void CDialogGameOSD::UpdatePlayers()
{
  m_playerTimeout.Set(PLAYER_REFRESH_INTERVAL);

  if (!CServiceBroker::IsServiceManagerUp())
    return;

  // List the players whose controller reports a battery level
  std::vector<std::pair<unsigned int, PERIPHERALS::PeripheralBattery>> players;

  const PERIPHERALS::PeripheralVector peripherals =
      CServiceBroker::GetGameServices().AgentInput().GetPlayerPeripherals();
  for (unsigned int i = 0; i < peripherals.size(); ++i)
  {
    if (!peripherals[i])
      continue;

    const PERIPHERALS::PeripheralBattery battery = peripherals[i]->GetBattery();
    if (battery.level >= 0 && battery.state != PERIPHERALS::PeripheralBatteryState::WIRED)
      players.emplace_back(i + 1, battery);
  }

  if (players == m_players)
    return;

  m_players = std::move(players);

  m_playerItems->Clear();
  for (const auto& [playerNumber, battery] : m_players)
  {
    auto item = std::make_shared<CFileItem>(std::to_string(playerNumber));
    item->SetProperty("game.batterylevel", battery.level);
    item->SetProperty("game.batterystate", BatteryStateToString(battery.state));
    m_playerItems->Add(std::move(item));
  }

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PLAYER_LIST, 0, 0, m_playerItems.get());
  OnMessage(msg);
}
