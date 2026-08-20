/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientJoystick.h"

#include "GameClientInput.h"
#include "GameClientTopology.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "games/ports/input/PortInput.h"
#include "input/joysticks/interfaces/IInputReceiver.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

#include <assert.h>

using namespace KODI;
using namespace GAME;

CGameClientJoystick::CGameClientJoystick(CGameClient& gameClient,
                                         const std::string& portAddress,
                                         const ControllerPtr& controller)
  : m_gameClient(gameClient),
    m_portAddress(portAddress),
    m_controller(controller),
    m_portInput(new CPortInput(this))
{
  assert(m_controller.get() != NULL);
}

CGameClientJoystick::~CGameClientJoystick() = default;

void CGameClientJoystick::RegisterInput(JOYSTICK::IInputProvider* inputProvider)
{
  m_portInput->RegisterInput(inputProvider);
}

void CGameClientJoystick::UnregisterInput(JOYSTICK::IInputProvider* inputProvider)
{
  m_portInput->UnregisterInput(inputProvider);
}

std::string CGameClientJoystick::ControllerID(void) const
{
  return m_controller->ID();
}

bool CGameClientJoystick::HasFeature(const std::string& feature) const
{
  return m_gameClient.Input().HasFeature(m_controller->ID(), feature);
}

bool CGameClientJoystick::AcceptsInput(const std::string& feature) const
{
  return m_gameClient.Input().AcceptsInput();
}

bool CGameClientJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.digital_button.pressed = bPressed;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientJoystick::OnButtonMotion(const std::string& feature,
                                         float magnitude,
                                         unsigned int motionTimeMs)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_ANALOG_BUTTON;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.analog_button.magnitude = magnitude;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientJoystick::OnAnalogStickMotion(const std::string& feature,
                                              float x,
                                              float y,
                                              unsigned int motionTimeMs)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_ANALOG_STICK;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.analog_stick.x = x;
  event.analog_stick.y = y;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientJoystick::OnAccelerometerMotion(const std::string& feature,
                                                float x,
                                                float y,
                                                float z)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_ACCELEROMETER;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.accelerometer.x = x;
  event.accelerometer.y = y;
  event.accelerometer.z = z;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientJoystick::OnWheelMotion(const std::string& feature,
                                        float position,
                                        unsigned int motionTimeMs)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_AXIS;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.axis.position = position;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientJoystick::OnThrottleMotion(const std::string& feature,
                                           float position,
                                           unsigned int motionTimeMs)
{
  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type = GAME_INPUT_EVENT_AXIS;
  event.controller_id = controllerId.c_str();
  event.port_type = GAME_PORT_CONTROLLER;
  event.port_address = m_portAddress.c_str();
  event.feature_name = feature.c_str();
  event.axis.position = position;

  return m_gameClient.Input().InputEvent(event);
}

std::string CGameClientJoystick::GetControllerAddress() const
{
  return CGameClientTopology::MakeAddress(m_portAddress, m_controller->ID());
}

std::string CGameClientJoystick::GetSourceLocation() const
{
  if (m_sourcePeripheral)
    return m_sourcePeripheral->FileLocation();

  return "";
}

float CGameClientJoystick::GetActivation() const
{
  return m_portInput->GetActivation();
}

void CGameClientJoystick::SetSource(PERIPHERALS::PeripheralPtr sourcePeripheral)
{
  m_sourcePeripheral = std::move(sourcePeripheral);
}

void CGameClientJoystick::ClearSource()
{
  m_sourcePeripheral.reset();
}

JOYSTICK::IInputReceiver* CGameClientJoystick::InputReceiver(void)
{
  // Answer with the port's receiver, not this handler's. CPortInput is what
  // registers with the peripheral, so it is what the peripheral attaches its
  // receiver to; this class sits behind it and is never given one, so every
  // motor event a game produced was dropped, for every core.
  return m_portInput ? m_portInput->InputReceiver() : nullptr;
}

bool CGameClientJoystick::SetRumble(const std::string& feature, float magnitude)
{
  bool bHandled = false;

  JOYSTICK::IInputReceiver* receiver = InputReceiver();

  if (receiver != nullptr)
    bHandled = receiver->SetRumbleState(feature, magnitude);

  // Report the first motor event, and the first that fails, once each. A game
  // asking for rumble and not producing any is otherwise entirely silent: the
  // request is well-formed the whole way down and simply stops, either because
  // no receiver was attached to this handler or because the peripheral's button
  // map has no motor of that name for the connected device.
  // Only a non-zero magnitude says anything: a game stopping a motor it never
  // started is accepted just as readily, and reporting that as success is what
  // made an unconnected chain look connected.
  const bool bMeaningful = (magnitude > 0.0f);

  // A flag each, rather than one shared between the failures. The two failures
  // are not the same event and the harmless one comes first: a game that asks
  // before its port is wired would otherwise spend the only flag, and the
  // warning that names a real gap in the button map would never be printed.
  if (bMeaningful)
  {
    if (bHandled)
    {
      if (!m_bLoggedRumble)
      {
        CLog::Log(LOGDEBUG, "GAME: Rumble \"{}\" at {:0.2f} accepted by the peripheral", feature,
                  magnitude);
        m_bLoggedRumble = true;
      }
    }
    else if (receiver != nullptr)
    {
      // The chain is connected and the request was still turned down, which
      // means the button map has no motor of that name for this device
      if (!m_bLoggedRumbleFailure)
      {
        CLog::Log(LOGWARNING, "GAME: Rumble \"{}\" went nowhere, the peripheral would not take it",
                  feature);
        m_bLoggedRumbleFailure = true;
      }
    }
    else
    {
      // Ports are wired up after the client starts, so a game that asks this
      // early is told no and asks again once there is somewhere to send it.
      // Reporting that as a warning described a broken chain that then worked.
      if (!m_bLoggedRumbleUnwired)
      {
        CLog::Log(LOGDEBUG, "GAME: Rumble \"{}\" arrived before the port had a receiver", feature);
        m_bLoggedRumbleUnwired = true;
      }
    }
  }

  return bHandled;
}
