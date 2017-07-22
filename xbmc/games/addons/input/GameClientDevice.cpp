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

#include "GameClientPort.h"
#include "GameClientDevice.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/GameServices.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientDevice::CGameClientDevice(const game_input_device &device) :
  m_controller(GetController(device.controller_id))
{
  if (m_controller && device.available_ports != nullptr)
  {
    // Look for matching ports. We enumerate in physical order because logical
    // order can change per emulator.
    for (const auto &physicalPort : m_controller->Topology().Ports())
    {
      for (unsigned int i = 0; i < device.port_count; i++)
      {
        const auto &logicalPort = device.available_ports[i];
        if (logicalPort.port_id != nullptr && logicalPort.port_id == physicalPort.ID())
        {
          // Handle matching ports
          AddPort(logicalPort, physicalPort);
          break;
        }
      }
    }
  }
}

CGameClientDevice::CGameClientDevice(const ControllerPtr &controller) :
  m_controller(controller)
{
}

CGameClientDevice::~CGameClientDevice() = default;

void CGameClientDevice::AddPort(const game_input_port &logicalPort, const CControllerPort &physicalPort)
{
  std::unique_ptr<CGameClientPort> port(new CGameClientPort(logicalPort, physicalPort));
  m_ports.emplace_back(std::move(port));
}

ControllerPtr CGameClientDevice::GetController(const char *controllerId)
{
  ControllerPtr controller;

  if (controllerId != nullptr)
    controller = CServiceBroker::GetGameServices().GetController(controllerId);

  return controller;
}
