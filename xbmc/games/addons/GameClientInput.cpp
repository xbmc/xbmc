/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "GameClientInput.h"
#include "GameClient.h"
#include "games/controllers/Controller.h"
#include "input/joysticks/IInputReceiver.h"
#include "utils/log.h"

#include <algorithm>
#include <assert.h>

using namespace KODI;
using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient* gameClient, int port, const ControllerPtr& controller, const KodiToAddonFuncTable_Game *dllStruct) :
  m_gameClient(gameClient),
  m_port(port),
  m_controller(controller),
  m_dllStruct(dllStruct)
{
  assert(m_gameClient != NULL);
  assert(m_controller.get() != NULL);
}

std::string CGameClientInput::ControllerID(void) const
{
  return m_controller->ID();
}

bool CGameClientInput::HasFeature(const std::string& feature) const
{
  try
  {
    return m_dllStruct->HasFeature(m_controller->ID().c_str(), feature.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in HasFeature()", m_gameClient->ID().c_str());
  }

  return false;
}

bool CGameClientInput::AcceptsInput(void)
{
  return m_gameClient->AcceptsInput();
}

JOYSTICK::INPUT_TYPE CGameClientInput::GetInputType(const std::string& feature) const
{
  const std::vector<CControllerFeature>& features = m_controller->Layout().Features();

  for (std::vector<CControllerFeature>::const_iterator it = features.begin(); it != features.end(); ++it)
  {
    if (feature == it->Name())
      return it->InputType();
  }

  return JOYSTICK::INPUT_TYPE::UNKNOWN;
}

bool CGameClientInput::OnButtonPress(const std::string& feature, bool bPressed)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                   = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.port                   = m_port;
  event.controller_id          = controllerId.c_str();
  event.feature_name           = feature.c_str();
  event.digital_button.pressed = bPressed;

  try
  {
    bHandled = m_dllStruct->InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
  }

  return bHandled;
}

bool CGameClientInput::OnButtonMotion(const std::string& feature, float magnitude)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type                    = GAME_INPUT_EVENT_ANALOG_BUTTON;
  event.port                    = m_port;
  event.controller_id           = controllerId.c_str();
  event.feature_name            = feature.c_str();
  event.analog_button.magnitude = magnitude;

  try
  {
    bHandled = m_dllStruct->InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
  }

  return bHandled;
}

bool CGameClientInput::OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs /* = 0 */)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type           = GAME_INPUT_EVENT_ANALOG_STICK;
  event.port           = m_port;
  event.controller_id  = controllerId.c_str();
  event.feature_name   = feature.c_str();
  event.analog_stick.x = x;
  event.analog_stick.y = y;

  try
  {
    bHandled = m_dllStruct->InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
  }

  return bHandled;
}

bool CGameClientInput::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  bool bHandled = false;

  game_input_event event;

  std::string controllerId = m_controller->ID();

  event.type            = GAME_INPUT_EVENT_ACCELEROMETER;
  event.port            = m_port;
  event.controller_id   = controllerId.c_str();
  event.feature_name    = feature.c_str();
  event.accelerometer.x = x;
  event.accelerometer.y = y;
  event.accelerometer.z = z;

  try
  {
    bHandled = m_dllStruct->InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
  }

  return bHandled;
}

bool CGameClientInput::SetRumble(const std::string& feature, float magnitude)
{
  bool bHandled = false;

  if (InputReceiver())
    bHandled = InputReceiver()->SetRumbleState(feature, magnitude);

  return bHandled;
}
