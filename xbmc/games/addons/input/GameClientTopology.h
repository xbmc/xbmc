/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/controllers/types/ControllerTree.h"

#include <memory>

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CGameClientTopology
{
public:
  CGameClientTopology() = default;
  CGameClientTopology(GameClientPortVec ports, int playerLimit);

  void Clear();

  int GetPlayerLimit() const { return m_playerLimit; }

  const CControllerTree& GetControllerTree() const { return m_controllers; }
  CControllerTree& GetControllerTree() { return m_controllers; }

  // Utility functions
  static std::string MakeAddress(const std::string& baseAddress, const std::string& nodeId);
  static std::pair<std::string, std::string> SplitAddress(const std::string& nodeAddress);

private:
  static CControllerTree GetControllerTree(const GameClientPortVec& ports);
  static CPortNode GetPortNode(const GameClientPortPtr& port, const std::string& controllerAddress);
  static CControllerNode GetControllerNode(const GameClientDevicePtr& device,
                                           const std::string& portAddress);

  // Game API parameters
  GameClientPortVec m_ports;
  int m_playerLimit = -1;

  // Controller parameters
  CControllerTree m_controllers;
};

} // namespace GAME
} // namespace KODI
