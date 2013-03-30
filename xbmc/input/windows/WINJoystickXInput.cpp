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


CJoystickXInput::CJoystickXInput(unsigned int controllerID, unsigned int id)
  : m_state(), m_controllerID(controllerID), m_dwPacketNumber(0)
{
  m_state.id          = id;
  m_state.name        = "XBMC-Compatible XInput Controller";
  m_state.buttonCount = std::min(m_state.buttonCount, 10U);
  m_state.hatCount    = std::min(m_state.hatCount, 1U);
  m_state.axisCount   = std::min(m_state.axisCount, 5U);
}

/* static */
void CJoystickXInput::Initialize(JoystickArray &joysticks)
{
  DeInitialize(joysticks);

  XINPUT_STATE controllerState; // No need to memset, only checking for controller existence

  for (unsigned int i = 0; i < 4; i++)
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
  m_state.hats[0].up = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ? 1 : 0;
  m_state.hats[0].right = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ? 1 : 0;
  m_state.hats[0].down = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ? 1 : 0;
  m_state.hats[0].left = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ? 1 : 0;

  m_state.buttons[0] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A ? 1 : 0;
  m_state.buttons[1] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_B ? 1 : 0;
  m_state.buttons[2] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_X ? 1 : 0;
  m_state.buttons[3] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_Y ? 1 : 0;
  m_state.buttons[4] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? 1 : 0;
  m_state.buttons[5] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? 1 : 0;
  m_state.buttons[6] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ? 1 : 0;
  m_state.buttons[7] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_START ? 1 : 0;
  m_state.buttons[8] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ? 1 : 0;
  m_state.buttons[9] = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ? 1 : 0;

  m_state.NormalizeAxis(0, controllerState.Gamepad.sThumbLX, 32768);
  m_state.NormalizeAxis(1, -controllerState.Gamepad.sThumbLY, 32768);
  m_state.NormalizeAxis(2, (long)controllerState.Gamepad.bLeftTrigger -
    (long)controllerState.Gamepad.bRightTrigger, 255); // Combine into a single axis, like DirectInput
  m_state.NormalizeAxis(3, controllerState.Gamepad.sThumbRX, 32768);
  m_state.NormalizeAxis(4, -controllerState.Gamepad.sThumbRY, 32768);
}
