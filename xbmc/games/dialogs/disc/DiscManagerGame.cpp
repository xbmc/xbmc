/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerGame.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

void CDiscManagerGame::Initialize(GameClientPtr gameClient)
{
  m_gameClient = std::move(gameClient);
  m_initialDiscModel.Clear();

  if (m_gameClient)
  {
    if (m_gameClient->SupportsDiscControl())
    {
      // Refresh discs from live core state
      m_gameClient->Discs().RefreshDiscState();

      // Store initial disc state
      m_initialDiscModel = m_gameClient->Discs().GetDiscs();
    }
    else
    {
      CLog::Log(LOGERROR, "Game client does not support disc control. The Disc Manager dialog will "
                          "not function correctly.");
    }
  }
  else
  {
    CLog::Log(LOGERROR,
              "No active game client. The Disc Manager dialog will not function correctly.");
  }
}

void CDiscManagerGame::Deinitialize()
{
  // Handle disc state transitions
  if (m_gameClient && m_gameClient->SupportsDiscControl())
  {
    const CGameClientDiscModel& currentModel = m_gameClient->Discs().GetDiscs();

    // If selected disc changed, commit that change by forcing tray closed
    const std::string initialSelectedDisc = m_initialDiscModel.GetSelectedDiscPath();
    const std::string currentSelectedDisc = currentModel.GetSelectedDiscPath();
    const bool selectedDiscChanged = (initialSelectedDisc != currentSelectedDisc);

    // If the disc list has changed (a disc was added, removed or swapped),
    // force the tray closed
    const std::vector<GameClientDiscEntry>& initialDiscs = m_initialDiscModel.GetDiscs();
    const std::vector<GameClientDiscEntry>& currentDiscs = currentModel.GetDiscs();
    bool discListChanged = (initialDiscs != currentDiscs);

    if ((selectedDiscChanged || discListChanged) && m_gameClient->Discs().IsEjected())
      m_gameClient->Discs().SetEjected(false);
  }

  m_gameClient.reset();
  m_initialDiscModel.Clear();
}

bool CDiscManagerGame::IsEjected() const
{
  return m_gameClient && m_gameClient->Discs().IsEjected();
}

void CDiscManagerGame::GetState(bool& ejected, std::string& selectedDisc) const
{
  // Get ejected state
  ejected = IsEjected();

  // Get selected disc
  selectedDisc.clear();
  if (m_gameClient)
  {
    const CGameClientDiscModel& discModel = m_gameClient->Discs().GetDiscs();
    if (discModel.IsSelectedNoDisc())
    {
      auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
      selectedDisc = strings.Get(35274); // "No disc"
    }
    else
    {
      selectedDisc = discModel.GetSelectedDiscLabel();
    }
  }
}

unsigned int CDiscManagerGame::GetSelectedIndex(std::optional<size_t> selectedIndex,
                                                bool allowSelectNoDisc) const
{
  if (!m_gameClient)
    return 0;

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  unsigned int itemIndex = 0;

  for (size_t i = 0; i < discList.Size(); ++i)
  {
    // Hidden from the visible list, so it does not consume a UI row
    if (discList.IsRemovedSlotByIndex(i))
      continue;

    if (selectedIndex.has_value() && *selectedIndex == i)
      return itemIndex;

    ++itemIndex;
  }

  // If no real slot matched, select the appended "No disc" row when present
  if (allowSelectNoDisc)
    return itemIndex;

  // Fallback for delete flow or invalid selection
  return 0;
}

std::optional<std::string> CDiscManagerGame::GetDiscPathByIndex(
    std::optional<size_t> discIndex) const
{
  if (!m_gameClient || !discIndex.has_value())
    return std::nullopt;

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();
  if (*discIndex >= discList.Size() || discList.IsRemovedSlotByIndex(*discIndex))
    return std::nullopt;

  return discList.GetPathByIndex(*discIndex);
}

GameClientPtr CDiscManagerGame::GetGameClient()
{
  auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
  if (gameSettingsHandle)
  {
    ADDON::AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                               ADDON::AddonType::GAMEDLL,
                                               ADDON::OnlyEnabled::CHOICE_YES))
    {
      return std::static_pointer_cast<CGameClient>(addon);
    }
  }

  return {};
}
