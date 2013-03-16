/*
 *      Copyright (C) 2007-2013 Team XBMC
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

#pragma once

#include "IJoystick.h"
#include "utils/StdString.h"

class CAction;

// Class to manage all connected joysticks
class CJoystickManager
{
private:
  CJoystickManager() : m_bEnabled(true), m_bWakeupChecked(false), m_actionTracker() { }
  ~CJoystickManager() { DeInitialize(); }

public:
  static CJoystickManager &Get();

  void SetEnabled(bool enabled = true);
  bool IsEnabled() const { return m_bEnabled; }
  void Update();
  unsigned int Count() const { return m_joysticks.size(); }
  void Reinitialize() { Initialize(); }
  void Reset() { m_actionTracker.Reset(); }

private:
  void Initialize();
  void DeInitialize();

  void ProcessStateChanges();
  void ProcessButtonPresses(SJoystick &oldState, const SJoystick &newState, unsigned int joyID);
  void ProcessHatPresses(SJoystick &oldState, const SJoystick &newState, unsigned int joyID);
  void ProcessAxisMotion(SJoystick &oldState, const SJoystick &newState, unsigned int joyID);

  // Returns true if this wakes up from the screensaver
  bool Wakeup();
  // Allows Wakeup() to perform another wakeup check
  void ResetWakeup() { m_bWakeupChecked = false; }

  JoystickArray m_joysticks;
  SJoystick     m_states[GAMEPAD_MAX_CONTROLLERS];
  bool          m_bEnabled;
  bool          m_bWakeupChecked; // true if WakeupCheck() has been called

  struct ActionTracker
  {
    ActionTracker() { Reset(); }
    void Reset() { actionID = targetTime = 0; name.clear(); }
    void Track(const CAction &action);
    int        actionID;
    CStdString name;
    uint32_t   targetTime;
  };

  ActionTracker m_actionTracker;
};
