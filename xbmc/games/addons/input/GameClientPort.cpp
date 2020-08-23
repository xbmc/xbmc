/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientPort.h"

#include "GameClientDevice.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/addons/GameClientTranslator.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"
#include "utils/StringUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientPort::CGameClientPort(const game_input_port& port)
  : m_type(CGameClientTranslator::TranslatePortType(port.type)),
    m_portId(port.port_id ? port.port_id : "")
{
  if (port.accepted_devices != nullptr)
  {
    for (unsigned int i = 0; i < port.device_count; i++)
    {
      std::unique_ptr<CGameClientDevice> device(new CGameClientDevice(port.accepted_devices[i]));

      if (device->Controller() != CController::EmptyPtr)
        m_acceptedDevices.emplace_back(std::move(device));
    }
  }
}

CGameClientPort::CGameClientPort(const ControllerVector& controllers)
  : m_type(PORT_TYPE::CONTROLLER), m_portId(DEFAULT_PORT_ID)
{
  for (const auto& controller : controllers)
    m_acceptedDevices.emplace_back(new CGameClientDevice(controller));
}

CGameClientPort::CGameClientPort(const game_input_port& logicalPort,
                                 const CControllerPort& physicalPort)
  : m_type(PORT_TYPE::CONTROLLER), m_portId(physicalPort.ID())
{
  if (logicalPort.accepted_devices != nullptr)
  {
    for (unsigned int i = 0; i < logicalPort.device_count; i++)
    {
      // Ensure device is physically compatible
      const game_input_device& deviceStruct = logicalPort.accepted_devices[i];
      std::string controllerId = deviceStruct.controller_id ? deviceStruct.controller_id : "";

      if (physicalPort.IsCompatible(controllerId))
      {
        std::unique_ptr<CGameClientDevice> device(new CGameClientDevice(deviceStruct));

        if (device->Controller() != CController::EmptyPtr)
          m_acceptedDevices.emplace_back(std::move(device));
      }
    }
  }
}

CGameClientPort::~CGameClientPort() = default;
