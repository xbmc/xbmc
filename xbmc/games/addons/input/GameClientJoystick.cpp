/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientJoystick.h"
#include "GameClientInput.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "games/ports/Port.h"
#include "input/joysticks/interfaces/IInputReceiver.h"
#include "utils/log.h"

#include <assert.h>

using namespace KODI;
using namespace GAME;

CGameClientJoystick::CGameClientJoystick(const CGameClient &gameClient,
                                         const std::string &portAddress,
                                         const ControllerPtr& controller,
                                         const KodiToAddonFuncTable_Game &dllStruct) :
  m_gameClient(gameClient),
  m_portAddress(portAddress),
  m_controller(controller),
  m_dllStruct(dllStruct),
  m_port(new CPort(this))
{
  assert(m_controller.get() != NULL);
}

CGameClientJoystick::~CGameClientJoystick() = default;

void CGameClientJoystick::RegisterInput(JOYSTICK::IInputProvider *inputProvider)
{
  m_port->RegisterInput(inputProvider);
}

void CGameClientJoystick::UnregisterInput(JOYSTICK::IInputProvider *inputProvider)
{
  m_port->UnregisterInput(inputProvider);
}

std::string CGameClientJoystick::ControllerID(void) const
{
  return m_controller->ID();
}

bool CGameClientJoystick::HasFeature(const std::string& feature) const
{
  try
  {
    return m_dllStruct.HasFeature(m_controller->ID().c_str(), feature.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in HasFeature()", m_gameClient.ID().c_str());
  }

  return false;
}

bool CGameClientJoystick::AcceptsInput(const std::string &feature) const
{
  return m_gameClient.Input().AcceptsInput();
}

bool CGameClientJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                   = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.controller_id          = controllerId.c_str();
  event.port_type              = GAME_PORT_CONTROLLER;
  event.port_address           = m_portAddress.c_str();
  event.feature_name           = feature.c_str();
  event.digital_button.pressed = bPressed;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                    = GAME_INPUT_EVENT_ANALOG_BUTTON;
  event.controller_id           = controllerId.c_str();
  event.port_type               = GAME_PORT_CONTROLLER;
  event.port_address            = m_portAddress.c_str();
  event.feature_name            = feature.c_str();
  event.analog_button.magnitude = magnitude;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type           = GAME_INPUT_EVENT_ANALOG_STICK;
  event.controller_id  = controllerId.c_str();
  event.port_type      = GAME_PORT_CONTROLLER;
  event.port_address   = m_portAddress.c_str();
  event.feature_name   = feature.c_str();
  event.analog_stick.x = x;
  event.analog_stick.y = y;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type            = GAME_INPUT_EVENT_ACCELEROMETER;
  event.controller_id   = controllerId.c_str();
  event.port_type       = GAME_PORT_CONTROLLER;
  event.port_address    = m_portAddress.c_str();
  event.feature_name    = feature.c_str();
  event.accelerometer.x = x;
  event.accelerometer.y = y;
  event.accelerometer.z = z;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                    = GAME_INPUT_EVENT_AXIS;
  event.controller_id           = controllerId.c_str();
  event.port_type               = GAME_PORT_CONTROLLER;
  event.port_address            = m_portAddress.c_str();
  event.feature_name            = feature.c_str();
  event.axis.position           = position;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught while handling wheel \"%s\"",
              m_gameClient.ID().c_str(), feature.c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                    = GAME_INPUT_EVENT_AXIS;
  event.controller_id           = controllerId.c_str();
  event.port_type               = GAME_PORT_CONTROLLER;
  event.port_address            = m_portAddress.c_str();
  event.feature_name            = feature.c_str();
  event.axis.position           = position;

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught while handling throttle \"%s\"",
              m_gameClient.ID().c_str(), feature.c_str());
  }

  return bHandled;
}

bool CGameClientJoystick::SetRumble(const std::string& feature, float magnitude)
{
  bool bHandled = false;

  if (InputReceiver())
    bHandled = InputReceiver()->SetRumbleState(feature, magnitude);

  return bHandled;
}
