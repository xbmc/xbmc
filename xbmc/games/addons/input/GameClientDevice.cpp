/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDevice.h"

#include "GameClientPort.h"
#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientDevice::CGameClientDevice(const game_input_device& device)
  : m_controller(GetController(device.controller_id))
{
  if (m_controller && device.available_ports != nullptr)
  {
    // Look for matching ports. We enumerate in physical order because logical
    // order can change per emulator.
    for (const auto& physicalPort : m_controller->Topology().Ports())
    {
      for (unsigned int i = 0; i < device.port_count; i++)
      {
        const auto& logicalPort = device.available_ports[i];
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

CGameClientDevice::CGameClientDevice(const ControllerPtr& controller) : m_controller(controller)
{
}

CGameClientDevice::~CGameClientDevice() = default;

void CGameClientDevice::AddPort(const game_input_port& logicalPort,
                                const CPhysicalPort& physicalPort)
{
  std::unique_ptr<CGameClientPort> port(new CGameClientPort(logicalPort, physicalPort));
  m_ports.emplace_back(std::move(port));
}

ControllerPtr CGameClientDevice::GetController(const char* controllerId)
{
  ControllerPtr controller;

  if (controllerId != nullptr)
  {
    controller = CServiceBroker::GetGameServices().GetController(controllerId);
    if (!controller)
      CLog::Log(LOGERROR, "Invalid controller ID: {}", controllerId);
  }

  return controller;
}
