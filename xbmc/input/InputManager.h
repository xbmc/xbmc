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
#include <memory>
#include <string>
#include <vector>

#if defined(HAS_LIRC)
#include "input/linux/LIRC.h"
#endif
#if defined(HAS_IRSERVERSUITE)
#include "input/windows/IRServerSuite.h"
#endif

#include "Action.h"
#include "windowing/XBMC_events.h"
#include "input/keyboard/IKeyboardInputProvider.h"
#include "input/mouse/IMouseInputProvider.h"
#include "input/KeyboardStat.h"
#include "input/MouseStat.h"
#include "interfaces/IActionListener.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

class CAppParamParser;
class CButtonTranslator;
class CCustomControllerTranslator;
class CIRTranslator;
class CJoystickMapper;
class CKey;
class CTouchTranslator;
class IKeymapEnvironment;
class IWindowKeymap;

namespace KODI
{
namespace KEYBOARD
{
  class IKeyboardHandler;
}

namespace MOUSE
{
  class IMouseButtonMap;
  class IMouseDriverHandler;
  class IMouseInputHandler;
}
}

/// \addtogroup input
/// \{

/*!
 * \ingroup input keyboard mouse touch joystick
 * \brief Main input processing class.
 *
 * This class consolidates all input generated from different sources such as
 * mouse, keyboard, joystick or touch (in \ref OnEvent).
 *
 * \copydoc keyboard
 * \copydoc mouse
 */
class CInputManager : public ISettingCallback,
                      public IActionListener,
                      public KODI::KEYBOARD::IKeyboardInputProvider,
                      public KODI::MOUSE::IMouseInputProvider,
                      public Observable
{
public:
  CInputManager(const CAppParamParser &params);
  CInputManager(const CInputManager&) = delete;
  CInputManager const& operator=(CInputManager const&) = delete;
  ~CInputManager() override;

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

  /*!
   * \brief Deinitialize input and keymaps
   */
  void Deinitialize();

  /*! \brief Handle an input event
   * 
   * \param newEvent event details
   * \return true on successfully handled event
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

  /*! \brief Returns whether or not we can handle a given built-in command. */

  bool HasBuiltin(const std::string& command);

  /*! \brief Parse a builtin command and execute any input action
   *  currently only LIRC commands implemented
   *
   * \param[in] execute Command to execute
   * \param[in] params  parameters that was passed to the command
   * \return 0 on success, -1 on failure
   */
  int ExecuteBuiltin(const std::string& execute, const std::vector<std::string>& params);

  // Button translation
  bool LoadKeymaps();
  bool ReloadKeymaps();
  void ClearKeymaps();
  void AddKeymap(const std::string &keymap);
  void RemoveKeymap(const std::string &keymap);

  const IKeymapEnvironment *KeymapEnvironment() const { return m_keymapEnvironment.get(); }

  /*! \brief Obtain the action configured for a given window and key
   *
   * \param window the window id
   * \param key the key to query the action for
   * \param fallback if no action is directly configured for the given window, obtain the action from fallback window, if exists or from global config as last resort
   *
   * \return the action matching the key
   */
  CAction GetAction(int window, const CKey &key, bool fallback = true);

  /*! \brief Obtain the global action configured for a given key
   *
   * \param key the key to query the action for
   *
   * \return the global action
   */
  CAction GetGlobalAction(const CKey &key);

  bool TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction);

  bool TranslateTouchAction(int windowId, int touchAction, int touchPointers, int &action, std::string &actionString);

  std::vector<std::shared_ptr<const IWindowKeymap>> GetJoystickKeymaps() const;

  int TranslateLircRemoteString(const std::string &szDevice, const std::string &szButton);

  // implementation of ISettingCallback
  virtual void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  // implementation of IActionListener
  virtual bool OnAction(const CAction& action) override;

  // implementation of IKeyboardInputProvider
  virtual void RegisterKeyboardHandler(KODI::KEYBOARD::IKeyboardHandler* handler) override;
  virtual void UnregisterKeyboardHandler(KODI::KEYBOARD::IKeyboardHandler* handler) override;

  // implementation of IMouseInputProvider
  virtual std::string RegisterMouseHandler(KODI::MOUSE::IMouseInputHandler* handler) override;
  virtual void UnregisterMouseHandler(KODI::MOUSE::IMouseInputHandler* handler) override;

private:

  /*! \brief Process keyboard event and translate into an action
  *
  * \param CKey keypress details
  * \return true on successfully handled event
  * \sa CKey
  */
  bool OnKey(const CKey& key);

  /*! \brief Process key up event
   *
   * \param CKey details of released key
   * \sa CKey
   */
  void OnKeyUp(const CKey& key);

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

  /*! \brief Dispatch actions queued since the last call to Process()
   */
  void ProcessQueuedActions();

  /*! \brief Queue an action to be processed on the next call to Process()
   */
  void QueueAction(const CAction& action);

  CKeyboardStat m_Keyboard;
  CMouseStat m_Mouse;
  CKey m_LastKey;

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CRemoteControl m_RemoteControl;
#endif

#if defined(HAS_EVENT_SERVER)
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

  std::vector<CAction> m_queuedActions;
  CCriticalSection     m_actionMutex;

  // Button translation
  std::unique_ptr<IKeymapEnvironment> m_keymapEnvironment;
  std::unique_ptr<CButtonTranslator> m_buttonTranslator;
  std::unique_ptr<CIRTranslator> m_irTranslator;
  std::unique_ptr<CCustomControllerTranslator> m_customControllerTranslator;
  std::unique_ptr<CTouchTranslator> m_touchTranslator;
  std::unique_ptr<CJoystickMapper> m_joystickTranslator;

  std::vector<KODI::KEYBOARD::IKeyboardHandler*> m_keyboardHandlers;

  struct MouseHandlerHandle
  {
    KODI::MOUSE::IMouseInputHandler*                  inputHandler;
    std::unique_ptr<KODI::MOUSE::IMouseDriverHandler> driverHandler;
  };

  std::vector<MouseHandlerHandle> m_mouseHandlers;
  std::unique_ptr<KODI::MOUSE::IMouseButtonMap> m_mouseButtonMap;

  std::unique_ptr<KODI::KEYBOARD::IKeyboardHandler> m_keyboardEasterEgg;
};

/// \}
