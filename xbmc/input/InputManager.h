#pragma once
/*
*      Copyright (C) 2005-2014 Team XBMC
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

#include <map>
#include <string>

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "input/SDLJoystick.h"
#endif
#include "windowing/XBMC_events.h"
#include "guilib/Key.h"

class CInputManager
{
private:
  CInputManager() { }
  CInputManager(const CInputManager&);
  CInputManager const& operator=(CInputManager const&);
  virtual ~CInputManager() { };

  friend class CSettings;

public:
  /*! \brief static method to get the current instance of the class. Creates a new instance the first time it's called.
  */
  static CInputManager& Get();

  /*! \brief decode an input event from remote controls.

   \param windowId Currently active window
   \return true if event is handled, false otherwise
  */
  bool ProcessRemote(int windowId);

  /*! \brief decode a mouse event and reset idle timers.

  \param windowId Currently active window
  \return true if event is handled, false otherwise
  */
  bool ProcessMouse(int windowId);

  /*! \brief decode a gamepad or joystick event, reset idle timers.

  \param windowId Currently active window
  \return true if event is handled, false otherwise
  */
  bool ProcessGamepad(int windowId);

  /*! \brief decode an event from the event service, this can be mouse, key, joystick, reset idle timers.

  \param windowId Currently active window
  \param frameTime Time in seconds since last call
  \return true if event is handled, false otherwise
  */
  bool ProcessEventServer(int windowId, float frameTime);

  /*! \brief decode an event from peripherals.

  \param frameTime Time in seconds since last call
  \return true if event is handled, false otherwise
  */
  bool ProcessPeripherals(float frameTime);

  /*!
   * \brief Call once during application startup to initialize peripherals that need it
   */
  void InitializeInputs();

  /*! \brief Enable or disable the joystick
   *
   * \param enabled true to enable joystick, false to disable
   * \return void
   */
  void SetEnabledJoystick(bool enabled = true);

  /*! \brief Run joystick initialization again, e.g. a new device is connected
  *
  * \return void
  */
  void ReInitializeJoystick();

  bool ProcessJoystickEvent(int windowId, const std::string& joystickName, int wKeyID, short inputType, float fAmount, unsigned int holdTime = 0);

#if defined(HAS_SDL_JOYSTICK) && !defined(TARGET_WINDOWS)
  void UpdateJoystick(SDL_Event& joyEvent);
#endif

  /*! \brief Handle an input event
   * 
   * \param newEvent event details
   * \return true on succesfully handled event
   * \sa XBMC_Event
   */
  bool OnEvent(XBMC_Event& newEvent);

private:

  /*! \brief Process keyboard event and translate into an action
  *
  * \param CKey keypress details
  * \return true on succesfully handled event
  * \sa CKey
  */
  bool OnKey(const CKey& key);

  /*! \brief Determine if an action should be processed or just
  *   cancel the screensaver
  *
  * \param action Action that is about to be processed
  * \return true on any poweractions such as shutdown/reboot/sleep/suspend, false otherwise
  * \sa CAction
  */
  bool AlwaysProcess(const CAction& action);

  /*! \brief Send the Action to CApplication for further handling,
  *   play a sound before or after sending the action.
  *
  * \param action Action to send to CApplication
  * \return result from CApplication::OnAction
  * \sa CAction
  */
  bool ExecuteInputAction(const CAction &action);

#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

#if defined(HAS_SDL_JOYSTICK) 
  CJoystick m_Joystick;
#endif
};
