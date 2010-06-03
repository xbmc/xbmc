#pragma once
/*
 *  Copyright (C) 2009-2010 Team XBMC
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

#include <windows.h>
#include <XInput.h>                       // Defines XBOX controller API
#pragma comment(lib, "XInput.lib")        // Library containing necessary 360
                                          // functions

class Xbox360Controller
{
private:
  XINPUT_STATE state;
  int num;
  bool button_down[14];
  bool button_pressed[14];
  bool button_released[14];

  XINPUT_STATE getState();
  void updateButton(int num, int button);
public:
  Xbox360Controller(int num);
  void updateState();
  bool isConnected();
  bool buttonPressed(int num);
  bool buttonReleased(int num);
  bool thumbMoved(int num);
  SHORT getThumb(int num);
  bool triggerMoved(int num);
  BYTE getTrigger(int num);
};
