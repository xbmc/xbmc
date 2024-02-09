/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentController.h"

#include "AgentJoystick.h"
#include "AgentKeyboard.h"
#include "AgentMouse.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CAgentController::CAgentController(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral))
{
  switch (m_peripheral->Type())
  {
    case PERIPHERALS::PERIPHERAL_JOYSTICK:
    {
      m_joystick = std::make_unique<CAgentJoystick>(m_peripheral);
      break;
    }
    case PERIPHERALS::PERIPHERAL_KEYBOARD:
    {
      m_keyboard = std::make_unique<CAgentKeyboard>(m_peripheral);
      break;
    }
    case PERIPHERALS::PERIPHERAL_MOUSE:
    {
      m_mouse = std::make_unique<CAgentMouse>(m_peripheral);
      break;
    }
    default:
      break;
  }
}

CAgentController::~CAgentController()
{
  Deinitialize();
}

void CAgentController::Initialize()
{
  if (m_joystick)
    m_joystick->Initialize();
  if (m_keyboard)
    m_keyboard->Initialize();
  if (m_mouse)
    m_mouse->Initialize();
}

void CAgentController::Deinitialize()
{
  if (m_mouse)
    m_mouse->Deinitialize();
  if (m_keyboard)
    m_keyboard->Deinitialize();
  if (m_joystick)
    m_joystick->Deinitialize();
}

std::string CAgentController::GetPeripheralName() const
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

const std::string& CAgentController::GetPeripheralLocation() const
{
  return m_peripheral->Location();
}

ControllerPtr CAgentController::GetController() const
{
  // Use joystick controller if joystick is initialized
  if (m_joystick)
  {
    ControllerPtr controller = m_joystick->Appearance();
    if (controller)
      return controller;
  }

  // Use peripheral controller if joystick is deinitialized
  return m_peripheral->ControllerProfile();
}

CDateTime CAgentController::LastActive() const
{
  return m_peripheral->LastActive();
}

float CAgentController::GetActivation() const
{
  // Return the maximum activation of all joystick, keyboard and mice input providers
  float activation = 0.0f;

  if (m_joystick)
    activation = std::max(activation, m_joystick->GetActivation());
  if (m_keyboard)
    activation = std::max(activation, m_keyboard->GetActivation());
  if (m_mouse)
    activation = std::max(activation, m_mouse->GetActivation());

  return activation;
}

void CAgentController::ClearButtonState()
{
  if (m_joystick)
    m_joystick->ClearButtonState();
  if (m_keyboard)
    m_keyboard->ClearButtonState();
  if (m_mouse)
    m_mouse->ClearButtonState();
}
