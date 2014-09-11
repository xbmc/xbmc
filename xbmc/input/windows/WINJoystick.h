#pragma once
/*
*      Copyright (C) 2012-2013 Team XBMC
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

#include <vector>
#include <string>
#include <map>
#include <stdint.h>
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

#define JACTIVE_BUTTON 0x00000001
#define JACTIVE_AXIS   0x00000002
#define JACTIVE_HAT    0x00000004
#define JACTIVE_NONE   0x00000000
#define JACTIVE_HAT_UP    0x01
#define JACTIVE_HAT_RIGHT 0x02
#define JACTIVE_HAT_DOWN  0x04
#define JACTIVE_HAT_LEFT  0x08

#define MAX_AXES 64

// Class to manage all connected joysticks

class CJoystick : public ISettingCallback
{
public:
  CJoystick();
  ~CJoystick();

  virtual void OnSettingChanged(const CSetting *setting);

  void Initialize();
  void Reset();
  void ResetAxis(int axisIdx) { m_Amount[axisIdx] = m_RestState[axisIdx]; }
  void Update();

  bool GetButton(std::string& joyName, int& id, bool consider_repeat = true);
  bool GetAxis(std::string& joyName, int& id);
  bool GetHat(std::string& joyName, int& id, int& position, bool consider_repeat = true);
  float GetAmount(std::string& joyName, int axisNum);

  bool IsEnabled() const { return m_joystickEnabled; }
  void SetEnabled(bool enabled = true);
  float SetDeadzone(float val);
  bool Reinitialize();
  void Acquire();
  void LoadTriggerMap(const std::map<std::string, std::vector<int> >& triggerMap);

private:
  bool IsButtonActive() { return m_ButtonIdx != -1; }
  bool IsAxisActive() { return m_AxisIdx != -1; }
  bool IsHatActive() { return m_HatIdx != -1; }

  void ReleaseJoysticks();

  int GetAxisWithMaxAmount();
  int JoystickIndex(std::string& joyName);
  int JoystickIndex(LPDIRECTINPUTDEVICE8 joy);
  int MapAxis(LPDIRECTINPUTDEVICE8 joy, int axisNum); // <joy, axis> --> axisIdx
  void MapAxis(int axisIdx, LPDIRECTINPUTDEVICE8& joy, int& axisNum); // axisIdx --> <joy, axis>
  int MapButton(LPDIRECTINPUTDEVICE8 joy, int buttonNum); // <joy, button> --> buttonIdx
  void MapButton(int buttonIdx, LPDIRECTINPUTDEVICE8& joy, int& buttonNum); // buttonIdx --> <joy, button>
  int MapHat(LPDIRECTINPUTDEVICE8 joy, int hatNum); // <joy, hat> --> hatIdx
  void MapHat(int hatIdx, LPDIRECTINPUTDEVICE8& joy, int& hatNum); // hatIdx --> <joy, hat>

  static BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
  static BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );

  int m_Amount[MAX_AXES];
  int m_RestState[MAX_AXES]; // axis value in rest state (0 for sticks, -32768 for triggers)
  bool m_IgnoreAxis[MAX_AXES]; // used to ignore triggers until SDL no longer reports 0
  int m_AxisIdx; // active axis
  int m_ButtonIdx; // active button
  uint8_t m_HatState; // state of active hat
  int m_HatIdx; // active hat

  int m_DeadzoneRange;
  bool m_joystickEnabled;
  uint32_t m_pressTicksButton;
  uint32_t m_pressTicksHat;
  CCriticalSection m_critSection;

  LPDIRECTINPUT8  m_pDI;
  std::vector<LPDIRECTINPUTDEVICE8> m_Joysticks;
  std::vector<std::string> m_JoystickNames;
  std::vector<DIDEVCAPS> m_devCaps;
  std::map<std::string, std::vector<int> > m_TriggerMap;
};

extern CJoystick g_Joystick;
