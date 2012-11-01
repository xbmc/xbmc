#pragma once
/*
*      Copyright (C) 2012 Team XBMC
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

#include <vector>
#include <string>
#include "threads/CriticalSection.h"

#define JACTIVE_BUTTON 0x00000001
#define JACTIVE_AXIS   0x00000002
#define JACTIVE_HAT    0x00000004
#define JACTIVE_NONE   0x00000000
#define JACTIVE_HAT_UP    0x01
#define JACTIVE_HAT_RIGHT 0x02
#define JACTIVE_HAT_DOWN  0x04
#define JACTIVE_HAT_LEFT  0x08

#define MAX_AXES 8

// Class to manage all connected joysticks

class CJoystick
{
public:
  CJoystick();
  ~CJoystick();

  void Initialize();
  void Reset(bool axis=false);
  void ResetAxis(int axisId) { m_Amount[axisId] = 0; }
  void Update();
  bool GetButton (int& id, bool consider_repeat=true);
  bool GetAxis (int &id);
  bool GetHat (int &id, int &position, bool consider_repeat=true);
  std::string GetJoystick() { return (m_JoyId>-1)?m_JoystickNames[m_JoyId]:""; }
  int GetAxisWithMaxAmount();
  float GetAmount(int axis);
  float GetAmount() { return GetAmount(m_AxisId); }
  bool IsEnabled() const { return m_joystickEnabled; }
  void SetEnabled(bool enabled = true);
  float SetDeadzone(float val);
  bool Reinitialize();
  void Acquire();

private:
  void SetAxisActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_AXIS):(m_ActiveFlags&(~JACTIVE_AXIS)); }
  void SetButtonActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_BUTTON):(m_ActiveFlags&(~JACTIVE_BUTTON)); }
  void SetHatActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_HAT):(m_ActiveFlags&(~JACTIVE_HAT)); }
  bool IsButtonActive() { return (m_ActiveFlags & JACTIVE_BUTTON) == JACTIVE_BUTTON; }
  bool IsAxisActive() { return (m_ActiveFlags & JACTIVE_AXIS) == JACTIVE_AXIS; }
  bool IsHatActive() { return (m_ActiveFlags & JACTIVE_HAT) == JACTIVE_HAT; }

  void ReleaseJoysticks();
  static BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
  static BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );

  int m_Amount[MAX_AXES];
  int m_AxisId;
  int m_ButtonId;
  uint8_t m_HatState;
  int m_HatId;
  int m_JoyId;
  int m_NumAxes;
  int m_DeadzoneRange;
  bool m_joystickEnabled;
  uint32_t m_pressTicksButton;
  uint32_t m_pressTicksHat;
  uint8_t m_ActiveFlags;
  uint32_t m_lastPressTicks;
  uint32_t m_lastTicks;
  CCriticalSection m_critSection;

  LPDIRECTINPUT8  m_pDI;
  std::vector<LPDIRECTINPUTDEVICE8> m_pJoysticks;
  std::vector<std::string> m_JoystickNames;
  std::vector<DIDEVCAPS> m_devCaps;
};

extern CJoystick g_Joystick;
