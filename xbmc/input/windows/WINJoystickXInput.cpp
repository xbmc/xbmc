/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "WINJoystickXInput.h"
#include "system.h"
#include "utils/log.h"

#include <windows.h>
#include <Xinput.h>

#pragma comment(lib, "XInput.lib")

#define XINPUT_ALIAS  "XBMC-Compatible XInput Controller"
#define MAX_JOYSTICKS 4
#define MAX_AXIS      32768
#define MAX_TRIGGER   255
#define BUTTON_COUNT  10
#define HAT_COUNT     1
#define AXIS_COUNT    5

using namespace JOYSTICK;

CJoystickXInput::CJoystickXInput(unsigned int controllerID, unsigned int id)
  : m_state(), m_controllerID(controllerID), m_dwPacketNumber(0)
{
  m_state.id = id;
  m_state.name = XINPUT_ALIAS;
  m_state.ResetState(BUTTON_COUNT, HAT_COUNT, AXIS_COUNT);
}

/* static */
void CJoystickXInput::Initialize(JoystickArray &joysticks)
{
  DeInitialize(joysticks);

  XINPUT_STATE controllerState; // No need to memset, only checking for controller existence

  for (unsigned int i = 0; i < MAX_JOYSTICKS; i++)
  {
    DWORD result = XInputGetState(i, &controllerState);
    if (result != ERROR_SUCCESS)
    {
      if (result == ERROR_DEVICE_NOT_CONNECTED)
        CLog::Log(LOGNOTICE, "CJoystickXInput: No XInput devices on port %u", i);
      continue;
    }

    // That's all it takes to check controller existence... I <3 XInput
    CLog::Log(LOGNOTICE, "CJoystickXInput: Found a 360-compatible XInput controller on port %u", i);
    joysticks.push_back(boost::shared_ptr<IJoystick>(new CJoystickXInput(i, joysticks.size())));
  }
}

/* static */
void CJoystickXInput::DeInitialize(JoystickArray &joysticks)
{
  for (int i = 0; i < (int)joysticks.size(); i++)
  {
    if (boost::dynamic_pointer_cast<CJoystickXInput>(joysticks[i]))
      joysticks.erase(joysticks.begin() + i--);
  }
}

void CJoystickXInput::Update()
{
  XINPUT_STATE controllerState;

  DWORD result = XInputGetState(m_controllerID, &controllerState);
  if (result != ERROR_SUCCESS)
    return;

  if (m_dwPacketNumber == controllerState.dwPacketNumber)
    return; // No update since last poll
  m_dwPacketNumber = controllerState.dwPacketNumber;

  // Map to DirectInput controls
  m_state.buttons[0] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? true : false;
  m_state.buttons[1] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? true : false;
  m_state.buttons[2] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_X) ? true : false;
  m_state.buttons[3] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ? true : false;
  m_state.buttons[4] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? true : false;
  m_state.buttons[5] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? true : false;
  m_state.buttons[6] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) ? true : false;
  m_state.buttons[7] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_START) ? true : false;
  m_state.buttons[8] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? true : false;
  m_state.buttons[9] = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? true : false;

  m_state.hats[0].up    = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? true : false;
  m_state.hats[0].right = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? true : false;
  m_state.hats[0].down  = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? true : false;
  m_state.hats[0].left  = (controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? true : false;

  // Combine triggers into a single axis, like DirectInput
  const long triggerAxis = (long)controllerState.Gamepad.bLeftTrigger - (long)controllerState.Gamepad.bRightTrigger;
  m_state.SetAxis(0, controllerState.Gamepad.sThumbLX, MAX_AXIS);
  m_state.SetAxis(1, -controllerState.Gamepad.sThumbLY, MAX_AXIS);
  m_state.SetAxis(2, triggerAxis, MAX_TRIGGER); 
  m_state.SetAxis(3, controllerState.Gamepad.sThumbRX, MAX_AXIS);
  m_state.SetAxis(4, -controllerState.Gamepad.sThumbRY, MAX_AXIS);
}
