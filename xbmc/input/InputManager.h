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
  static CInputManager& GetInstance();

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

  void SetEnabledJoystick(bool enabled = true);

  void ReInitializeJoystick();

  bool ProcessJoystickEvent(int windowId, const std::string& joystickName, int wKeyID, short inputType, float fAmount, unsigned int holdTime = 0);

#if defined(HAS_SDL_JOYSTICK) && !defined(TARGET_WINDOWS)
  void UpdateJoystick(SDL_Event& joyEvent);
#endif

private:
#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

#if defined(HAS_SDL_JOYSTICK) 
  CJoystick m_Joystick;
#endif
};
