/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgent.h"

#include "games/agents/input/GameAgentJoystick.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CGameAgent::CGameAgent(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral)),
    m_joystick(std::make_unique<CGameAgentJoystick>(m_peripheral))
{
  Initialize();
}

CGameAgent::~CGameAgent()
{
  Deinitialize();
}

void CGameAgent::Initialize()
{
  m_joystick->Initialize();
}

void CGameAgent::Deinitialize()
{
  m_joystick->Deinitialize();
}

std::string CGameAgent::GetPeripheralName() const
{
  std::string deviceName = m_peripheral->DeviceName();

  // Handle unknown device name
  if (deviceName.empty())
  {
    ControllerPtr controller = m_peripheral->ControllerProfile();
    if (controller)
      deviceName = controller->Layout().Label();
  }

  return deviceName;
}

const std::string& CGameAgent::GetPeripheralLocation() const
{
  return m_peripheral->Location();
}

ControllerPtr CGameAgent::GetController() const
{
  // Use joystick controller if joystick is initialized
  ControllerPtr controller = m_joystick->Appearance();
  if (controller)
    return controller;

  // Use peripheral controller if joystick is deinitialized
  return m_peripheral->ControllerProfile();
}

CDateTime CGameAgent::LastActive() const
{
  return m_peripheral->LastActive();
}

float CGameAgent::GetActivation() const
{
  return m_joystick->GetActivation();
}
