/*
 *  Copyright (C) 2009-2013 Team XBMC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "StdAfx.h"
#include "Xbox360Controller.h"


Xbox360Controller::Xbox360Controller(int num)
{
  this->num = num;
  for (int i = 0; i < 14; i++)
  {
    button_down[i] = false;
    button_released[i] = false;
    button_pressed[i] = false;
  }
}

XINPUT_STATE Xbox360Controller::getState()
{
    // Zeroise the state
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Get the state
    XInputGetState(num, &state);

    return state;
}

void Xbox360Controller::updateButton(int num, int button)
{
  if (state.Gamepad.wButtons & button)
  {
    if (!button_down[num])
    {
      button_pressed[num] = true;
    }
    button_down[num] = true;
  } else
  {
    if (button_down[num])
    {
      button_released[num] = true;
    }
    button_down[num] = false;
  }
}

bool Xbox360Controller::buttonPressed(int num)
{
  return button_pressed[num];
}

bool Xbox360Controller::buttonReleased(int num)
{
  return button_released[num];
}

void Xbox360Controller::updateState()
{
  for (int i = 0; i < 14; i++)
  {
    button_released[i] = false;
    button_pressed[i] = false;
  }
  if (isConnected())
  {
    XINPUT_STATE s = getState();
    updateButton(0, XINPUT_GAMEPAD_A);
    updateButton(1, XINPUT_GAMEPAD_B);
    updateButton(2, XINPUT_GAMEPAD_X);
    updateButton(3, XINPUT_GAMEPAD_Y);
    updateButton(4, XINPUT_GAMEPAD_DPAD_UP);
    updateButton(5, XINPUT_GAMEPAD_DPAD_DOWN);
    updateButton(6, XINPUT_GAMEPAD_DPAD_LEFT);
    updateButton(7, XINPUT_GAMEPAD_DPAD_RIGHT);
    updateButton(8, XINPUT_GAMEPAD_START);
    updateButton(9, XINPUT_GAMEPAD_BACK);
    updateButton(10, XINPUT_GAMEPAD_LEFT_THUMB);
    updateButton(11, XINPUT_GAMEPAD_RIGHT_THUMB);
    updateButton(12, XINPUT_GAMEPAD_LEFT_SHOULDER);
    updateButton(13, XINPUT_GAMEPAD_RIGHT_SHOULDER);
  }
}

bool Xbox360Controller::isConnected()
{
    // Zeroise the state
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Get the state
    DWORD Result = XInputGetState(num, &state);

    if(Result == ERROR_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool Xbox360Controller::triggerMoved(int num)
{
  if (num == 0)
    return (state.Gamepad.bRightTrigger &&
        state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
  return (state.Gamepad.bLeftTrigger &&
      state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

BYTE Xbox360Controller::getTrigger(int num)
{
  if (num == 0)
    return state.Gamepad.bRightTrigger;
  return state.Gamepad.bLeftTrigger;
}

bool Xbox360Controller::thumbMoved(int num)
{
  switch(num)
  {
  case 0:
    return !(state.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            state.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
  case 1:
    return !(state.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            state.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
  case 2:
    return !(state.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            state.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
  case 3:
    return !(state.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            state.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
  }

  return false;
}
SHORT Xbox360Controller::getThumb(int num)
{
  switch (num)
  {
  case 0:
    return state.Gamepad.sThumbLX;
  case 1:
    return state.Gamepad.sThumbLY;
  case 2:
    return state.Gamepad.sThumbRX;
  case 3:
    return state.Gamepad.sThumbRY;
  }

  return 0;
}

