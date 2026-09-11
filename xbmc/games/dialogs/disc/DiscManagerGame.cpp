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
#include "dialogs/GUIDialogKaiToast.h"
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
  m_pendingDiscModel.reset();

  if (m_gameClient)
  {
    if (m_gameClient->SupportsDiscControl())
    {
      // Refresh discs from live core state
      m_gameClient->Discs().RefreshDiscState();
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
  if (m_gameClient && m_gameClient->SupportsDiscControl())
  {
    const auto lock = m_gameClient->LockForSnapshot();
    CGameClientDiscs& discs = m_gameClient->Discs();

    // A savestate or rewind can supersede the last edit made by this dialog.
    if (m_pendingDiscModel && m_pendingRestoreGeneration == discs.GetRestoreGeneration() &&
        *m_pendingDiscModel == discs.GetDiscs() && discs.IsEjected() && !discs.SetEjected(false))
    {
      auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
      CLog::Log(LOGERROR, "Failed to insert selected disc when closing Disc Manager");
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, strings.Get(257),
                                            strings.Get(35279));
    }
  }

  m_gameClient.reset();
  m_pendingDiscModel.reset();
}

void CDiscManagerGame::NotifyDiscChange()
{
  if (m_gameClient)
  {
    const auto lock = m_gameClient->LockForSnapshot();
    m_pendingDiscModel = m_gameClient->Discs().GetDiscs();
    m_pendingRestoreGeneration = m_gameClient->Discs().GetRestoreGeneration();
  }
}

void CDiscManagerGame::NotifyTrayChange()
{
  m_pendingDiscModel.reset();
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
