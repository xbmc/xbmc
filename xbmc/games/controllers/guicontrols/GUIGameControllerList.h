/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIListContainer.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class CScroller;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIGameControllerList : public CGUIListContainer
{
public:
  CGUIGameControllerList(int parentID,
                         int controlID,
                         float posX,
                         float posY,
                         float width,
                         float height,
                         ORIENTATION orientation,
                         uint32_t alignment,
                         const CScroller& scroller);
  explicit CGUIGameControllerList(const CGUIGameControllerList& other);

  ~CGUIGameControllerList() override = default;

  // Implementation of CGUIControl via CGUIListContainer
  CGUIGameControllerList* Clone() const override;
  void UpdateInfo(const CGUIListItem* item) override;

  // Game properties
  void SetGameClient(GameClientPtr gameClient);
  void ClearGameClient();

  // GUI properties
  uint32_t GetAlignment() const { return m_alignment; }

private:
  void UpdatePort(int itemNumber, const std::vector<std::string>& inputPorts);
  void UpdatePortIndex(const PERIPHERALS::PeripheralPtr& agentPeripheral,
                       const std::vector<std::string>& inputPorts);
  void UpdatePeripheral(const PERIPHERALS::PeripheralPtr& agentPeripheral);

  static std::string GetPortAddress(const PERIPHERALS::PeripheralPtr& agentPeripheral);

  // Game properties
  GameClientPtr m_gameClient;
  unsigned int m_portCount{0};
  int m_portIndex{-1}; // Not connected
  std::string m_peripheralLocation;

  // GUI properties
  const uint32_t m_alignment;
};
} // namespace GAME
} // namespace KODI
