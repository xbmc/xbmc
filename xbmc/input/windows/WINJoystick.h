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
#include <list>
#include <memory>
#include <stdint.h>
#include "threads/CriticalSection.h"

#define JACTIVE_BUTTON 0x00000001
#define JACTIVE_AXIS   0x00000002
#define JACTIVE_HAT    0x00000004
#define JACTIVE_NONE   0x00000000
#define JACTIVE_HAT_UP    0x01
#define JACTIVE_HAT_RIGHT 0x02
#define JACTIVE_HAT_DOWN  0x04
#define JACTIVE_HAT_LEFT  0x08

struct AxisState {
  int val;
  int rest;
  bool trigger;
  AxisState() : val(0), rest(0), trigger(false) { }
  void reset() { val = rest; }
};

struct AxisConfig {
  int axis;
  bool isTrigger;
  int rest;
  AxisConfig(int axis, bool isTrigger, int rest) : axis(axis), isTrigger(isTrigger), rest(rest) { }
};

typedef std::vector<AxisConfig> AxesConfig; // [<axis, isTrigger, rest state value>]
class CRegExp;

// Class to manage all connected joysticks
class CJoystick
{
public:
  CJoystick();
  ~CJoystick();

  void Initialize();
  void Reset();
  void Update();

  bool GetButton(std::string &joyName, int &id, bool consider_repeat = true);
  bool GetAxis(std::string &joyName, int &id);
  bool GetAxes(std::list<std::pair<std::string, int> >& axes, bool consider_still = false);
  bool GetHat(std::string &joyName, int &id, int &position, bool consider_repeat = true);
  float GetAmount(const std::string &joyName, int axisNum) const;

  bool IsEnabled() const { return m_joystickEnabled; }
  void SetEnabled(bool enabled = true);
  float SetDeadzone(float val);
  bool Reinitialize();
  void Acquire();
  typedef std::vector<AxisConfig> AxesConfig; // [<axis, isTrigger, rest state value>]

private:
  bool IsButtonActive() const { return m_ButtonIdx != -1; }
  bool IsAxisActive() const { return m_AxisIdx != -1; }
  bool IsHatActive() const { return m_HatIdx != -1; }

  void ReleaseJoysticks();

  int GetAxisWithMaxAmount() const;
  int JoystickIndex(const std::string &joyName) const;
  int JoystickIndex(LPDIRECTINPUTDEVICE8 joy) const;
  int MapAxis(LPDIRECTINPUTDEVICE8 joy, int axisNum) const; // <joy, axis> --> axisIdx
  void MapAxis(int axisIdx, LPDIRECTINPUTDEVICE8 &joy, int &axisNum) const; // axisIdx --> <joy, axis>
  int MapButton(LPDIRECTINPUTDEVICE8 joy, int buttonNum) const; // <joy, button> --> buttonIdx
  void MapButton(int buttonIdx, LPDIRECTINPUTDEVICE8 &joy, int &buttonNum) const; // buttonIdx --> <joy, button>
  int MapHat(LPDIRECTINPUTDEVICE8 joy, int hatNum) const; // <joy, hat> --> hatIdx
  void MapHat(int hatIdx, LPDIRECTINPUTDEVICE8 &joy, int &hatNum) const; // hatIdx --> <joy, hat>

  static BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
  static BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );

  std::vector<AxisState> m_Axes;
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
  std::vector<LPDIRECTINPUTDEVICE8> m_pJoysticks;
  std::vector<std::string> m_JoystickNames;
  std::vector<DIDEVCAPS> m_devCaps;
};
