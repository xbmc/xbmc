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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "guilib/Key.h" // for ACTION_GAME_CONTROL_START
#include "input/IInputHandler.h"
#include "input/IJoystick.h" // for GAMEPAD_MAX_CONTROLLERS

#include <map>
#include <stdint.h>

class CRetroPlayerInput : public IInputHandler
{
public:
  CRetroPlayerInput() { Reset(); }
  void Reset();

  /**
   * Called by the game client to query gamepad states.
   * \param port   The player #. Player 1 is port 0.
   * \param device The fundamental device abstraction. This can be changed from
   *               the default by calling CGameClient::SetDevice().
   * \param index  Only used for analog devices (RETRO_DEVICE_ANALOG)
   * \param id     The button ID being queried.
   * \return       Although this returns int16_t, it seems game clients cast
   *               to bool. Therefore, 0 = not pressed, 1 = pressed.
   */
  int16_t GetInput(unsigned port, unsigned device, unsigned index, unsigned id);

  // Inherited from IInputHandler
  virtual void ProcessKeyDown(unsigned int controllerID, uint32_t key, const CAction &action);
  virtual void ProcessKeyUp(unsigned int controllerID, uint32_t key);
  virtual void ProcessButtonDown(unsigned int controllerID, unsigned int buttonID, const CAction &action);
  virtual void ProcessButtonUp(unsigned int controllerID, unsigned int buttonID);
  virtual void ProcessHatDown(unsigned int controllerID, unsigned int hatID, unsigned char dir, const CAction &action);
  virtual void ProcessHatUp(unsigned int controllerID, unsigned int hatID, unsigned char dir);
  virtual void ProcessAxis(unsigned int controllerID, unsigned int axisID, const CAction &action);

  struct DeviceItem
  {
    unsigned int controllerID;
    unsigned int key;
    unsigned int buttonID;
    unsigned int hatID;
    unsigned char hatDir;
    unsigned char axisID;
  };

private:
  /**
   * Translate an action ID, found in Key.h, to the corresponding RetroPad ID.
   * Returns -1 if the ID is invalid for the device given by m_device
   * (currently, m_device doesn't exist and this value is forced to
   * RETRO_DEVICE_JOYPAD).
   */
  int TranslateActionID(int id) const;

  int16_t m_joypadState[GAMEPAD_MAX_CONTROLLERS][ACTION_GAME_CONTROL_END - ACTION_GAME_CONTROL_START + 1];

  std::map<DeviceItem, int> m_deviceItems;
};
