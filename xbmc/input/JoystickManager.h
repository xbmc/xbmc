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
#include "settings/ISettingCallback.h"
#include "threads/SystemClock.h"

#include <string>

class CAction;

namespace JOYSTICK
{

/**
 * Track key presses for deferred action repeats.
 */
struct ActionTracker
{
  ActionTracker() { Reset(); }
  void Reset();
  void Track(const CAction &action);

  int                  actionID; // Action ID, or 0 if not tracking any action
  std::string          name; // Action name
  XbmcThreads::EndTime timeout; // Timeout until action is repeated
};

/**
 * Class to manage all connected joysticks.
 */
class CJoystickManager : public ISettingCallback
{
private:
  CJoystickManager() : m_bEnabled(false), m_bWakeupChecked(false) { }
  virtual ~CJoystickManager() { DeInitialize(); }

public:
  static CJoystickManager &Get();

  void SetEnabled(bool enabled = true);
  bool IsEnabled() const { return m_bEnabled; }
  void Update();
  unsigned int Count() const { return m_joysticks.size(); }
  void Reinitialize() { Initialize(); }
  void Reset() { m_actionTracker.Reset(); }

  // Inherited from ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);

private:
  void Initialize();
  void DeInitialize();

  /**
   * After updating, look for changes in state.
   * @param oldState - previous joystick state, set to newState as a post-condition
   * @param newState - the updated joystick state
   * @param joyID - the ID of the joystick being processed
   */
  void ProcessStateChanges();
  void ProcessButtonPresses(Joystick &oldState, const Joystick &newState, unsigned int joyID);
  void ProcessHatPresses(Joystick &oldState, const Joystick &newState, unsigned int joyID);
  void ProcessAxisMotion(Joystick &oldState, const Joystick &newState, unsigned int joyID);

  // Returns true if this wakes up from the screensaver
  bool Wakeup();
  // Allows Wakeup() to perform another wakeup check
  void ResetWakeup() { m_bWakeupChecked = false; }

  inline bool IsGameControl(int actionID);

  JoystickArray m_joysticks;
  Joystick      m_states[GAMEPAD_MAX_CONTROLLERS];
  bool          m_bEnabled;
  bool          m_bWakeupChecked; // true if WakeupCheck() has been called

  ActionTracker m_actionTracker;
};

} // namespace INPUT
