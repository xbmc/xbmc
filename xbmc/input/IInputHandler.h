/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#pragma once

class CAction;

class IInputHandler
{
public:
  /**
   * Marks a key as pressed. This intercepts keys sent to CApplication::OnKey()
   * before they are translated into actions.
   */
  virtual void ProcessKeyDown(unsigned int controllerID, uint32_t key, const CAction &action) = 0;

  /**
   * Marks a key as released. Because key releases aren't processed by
   * CApplication and aren't translated into actions, these are intercepted
   * at the raw event stage in CApplication::OnEvent().
   */
  virtual void ProcessKeyUp(unsigned int controllerID, uint32_t key) = 0;

  /**
   * Notify interface of joystick events from CJoystickManager.
   */
  virtual void ProcessButtonDown(unsigned int controllerID, unsigned int buttonID, const CAction &action) = 0;
  virtual void ProcessButtonUp(unsigned int controllerID, unsigned int buttonID) = 0;
  virtual void ProcessHatDown(unsigned int controllerID, unsigned int hatID, unsigned char dir, const CAction &action) = 0;
  virtual void ProcessHatUp(unsigned int controllerID, unsigned int hatID, unsigned char dir) = 0;
  virtual void ProcessAxis(unsigned int controllerID, unsigned int axisID, const CAction &action) = 0;
};
