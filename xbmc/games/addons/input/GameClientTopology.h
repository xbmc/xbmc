/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "games/controllers/types/ControllerTree.h"
#include "games/GameTypes.h"

#include <memory>

namespace KODI
{
namespace GAME
{
  class CGameClientTopology
  {
  public:
    CGameClientTopology(GameClientPortVec ports);

    CControllerTree GetControllerTree() const;

  private:
    static CControllerTree GetControllerTree(const GameClientPortVec &ports);
    static CControllerPortNode GetPortNode(const GameClientPortPtr &port, const std::string &address);
    static CControllerNode GetControllerNode(const GameClientDevicePtr &device, const std::string &portAddress);

    // Utility function
    static std::string MakeAddress(const std::string &baseAddress, const std::string &nodeId);

    GameClientPortVec m_ports;
  };
}
}
