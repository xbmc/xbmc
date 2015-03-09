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
#include <vector>

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "input/SDLJoystick.h"
#endif
#if defined(HAS_LIRC)
#include "input/linux/LIRC.h"
#endif
#if defined(HAS_IRSERVERSUITE)
#include "input/windows/IRServerSuite.h"
#endif

#include "windowing/XBMC_events.h"
#include "input/KeyboardStat.h"
#include "input/MouseStat.h"
#include "settings/lib/ISettingCallback.h"

class CKey;

class CInputManager : public ISettingCallback
{
private:
  CInputManager() { }
  CInputManager(const CInputManager&);
  CInputManager const& operator=(CInputManager const&);
  virtual ~CInputManager() { };

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

  /*! \brief Process all inputs
   *
   * \param windowId Currently active window
   * \param frameTime Time in seconds since last call
   * \return true on success, false otherwise
   */
  bool Process(int windowId, float frameTime);

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

  /*! \brief Control if the mouse is actively used or not
   *
   * \param[in] active sets mouse active or inactive
   */
  void SetMouseActive(bool active = true);

  /*! \brief Control if we should use a mouse or not
   *
   * \param[in] mouseEnabled sets mouse enabled or disabled
   */
  void SetMouseEnabled(bool mouseEnabled = true);

  /*! \brief Set the current state of the mouse such as click, drag operation
   *
   * \param[in] mouseState which state the mouse should be set to
   * \sa MOUSE_STATE
   */
  void SetMouseState(MOUSE_STATE mouseState);

  /*! \brief Check if the mouse is currently active
   *
   * \return true if active, false otherwise 
   */
  bool IsMouseActive();

  /*! \brief Get the current state of the mouse, such as click or drag operation
   *
   * \return the current state of the mouse as a value from MOUSE_STATE
   * \sa MOUSE_STATE
   */
  MOUSE_STATE GetMouseState();

  /*! \brief Get the current mouse positions x and y coordinates
   *
   * \return a struct containing the x and y coordinates
   * \sa MousePosition
   */
  MousePosition GetMousePosition();

  /*! \brief Set the current screen resolution and pointer speed
   *
   * \param[in] maxX    screen width
   * \param[in] maxY    screen height
   * \param[in] speedX  mouse speed in x dimension
   * \param[in] speedY  mouse speed in y dimension
   * \return 
   */
  void SetMouseResolution(int maxX, int maxY, float speedX, float speedY);

  /*! \brief Enable the remote control
   *
   */
  void EnableRemoteControl();

  /*! \brief Disable the remote control
   *
   */
  void DisableRemoteControl();

  /*! \brief Try to connect to a remote control to listen for commands
   *
   */
  void InitializeRemoteControl();

  /*! \brief Check if the remote control is enabled
   *
   * \return true if remote control is enabled, false otherwise 
   */
  bool IsRemoteControlEnabled();

  /*! \brief Check if the remote control is initialized
   *
   * \return true if initialized, false otherwise 
   */
  bool IsRemoteControlInitialized();

  /*! \brief Set the device name to use with LIRC, does nothing 
   *   if IRServerSuite is used
   *
   * \param[in] name Name of the device to use with LIRC
   */
  void SetRemoteControlName(const std::string& name);

  /*! \brief Parse a builtin command and execute any input action
   *  currently only LIRC commands implemented
   *
   * \param[in] execute Command to execute
   * \param[in] params  parameters that was passed to the command
   * \return 0 on success, -1 on failure
   */
  int ExecuteBuiltin(const std::string& execute, const std::vector<std::string>& params);

  virtual void OnSettingChanged(const CSetting *setting);

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

  CKeyboardStat m_Keyboard;
  CMouseStat m_Mouse;

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CRemoteControl m_RemoteControl;
#endif

#if defined(HAS_EVENT_SERVER)
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

#if defined(HAS_SDL_JOYSTICK) 
  CJoystick m_Joystick;
#endif
};
