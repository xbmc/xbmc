/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/Action.h"
#include "input/actions/interfaces/IActionListener.h"
#include "input/keyboard/KeyboardStat.h"
#include "input/keymaps/ButtonStat.h"
#include "input/mouse/MouseStat.h"
#include "input/mouse/interfaces/IMouseInputProvider.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "windowing/XBMC_events.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class CKey;
class CProfileManager;

namespace KODI
{

namespace KEYBOARD
{
class IKeyboardDriverHandler;
}

namespace KEYMAP
{
class CButtonTranslator;
class CCustomControllerTranslator;
class CJoystickMapper;
class CTouchTranslator;
class IKeymapEnvironment;
class IWindowKeymap;
} // namespace KEYMAP

namespace MOUSE
{
class IMouseDriverHandler;
}
} // namespace KODI

/// \addtogroup input
/// \{

/*!
 * \ingroup input keyboard mouse touch joystick keymap
 *
 * \brief Main input processing class.
 *
 * This class consolidates all input generated from different sources such as
 * mouse, keyboard, joystick or touch (in \ref OnEvent).
 *
 * \copydoc keyboard
 * \copydoc mouse
 */
class CInputManager : public ISettingCallback,
                      public KODI::ACTION::IActionListener,
                      public Observable
{
public:
  CInputManager();
  CInputManager(const CInputManager&) = delete;
  CInputManager const& operator=(CInputManager const&) = delete;
  ~CInputManager() override;

  /*! \brief decode a mouse event and reset idle timers.
   *
   * \param windowId Currently active window
   * \return true if event is handled, false otherwise
   */
  bool ProcessMouse(int windowId);

  /*! \brief decode an event from the event service, this can be mouse, key, joystick, reset idle
   * timers.
   *
   * \param windowId Currently active window
   * \param frameTime Time in seconds since last call
   * \return true if event is handled, false otherwise
   */
  bool ProcessEventServer(int windowId, float frameTime);

  /*! \brief decode an event from peripherals.
   *
   * \param frameTime Time in seconds since last call
   * \return true if event is handled, false otherwise
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

  /*! \brief Get the status of the controller-enable setting
   * \return True if controller input is enabled for the UI, false otherwise
   */
  bool IsControllerEnabled() const;

  /*! \brief Returns whether or not we can handle a given built-in command.
   *
   */
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
  void AddKeymap(const std::string& keymap);
  void RemoveKeymap(const std::string& keymap);

  const KODI::KEYMAP::IKeymapEnvironment* KeymapEnvironment() const;

  /*! \brief Obtain the action configured for a given window and key
   *
   * \param window the window id
   * \param key the key to query the action for
   * \param fallback if no action is directly configured for the given window, obtain the action
   * from fallback window, if exists or from global config as last resort
   *
   * \return the action matching the key
   */
  CAction GetAction(int window, const CKey& key, bool fallback = true);

  bool TranslateCustomControllerString(int windowId,
                                       const std::string& controllerName,
                                       int buttonId,
                                       int& action,
                                       std::string& strAction);

  bool TranslateTouchAction(
      int windowId, int touchAction, int touchPointers, int& action, std::string& actionString);

  std::vector<std::shared_ptr<const KODI::KEYMAP::IWindowKeymap>> GetJoystickKeymaps() const;

  /*!
   * \brief Queue an action to be processed on the next call to Process()
   */
  void QueueAction(const CAction& action);

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // implementation of IActionListener
  bool OnAction(const CAction& action) override;

  void RegisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler);
  void UnregisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler);

  virtual void RegisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler);
  virtual void UnregisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler);

private:
  /*! \brief Process keyboard event and translate into an action
   *
   * \param key keypress details
   * \return true on successfully handled event
   * \sa CKey
   */
  bool OnKey(const CKey& key);

  /*! \brief Process key up event
   *
   * \param key details of released key
   * \sa CKey
   */
  void OnKeyUp(const CKey& key);

  /*! \brief Handle keypress
   *
   * \param key keypress details
   * \return true on successfully handled event
   */
  bool HandleKey(const CKey& key);

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
  bool ExecuteInputAction(const CAction& action);

  /*! \brief Dispatch actions queued since the last call to Process()
   */
  void ProcessQueuedActions();

  KODI::KEYBOARD::CKeyboardStat m_Keyboard;
  KODI::KEYMAP::CButtonStat m_buttonStat;
  CMouseStat m_Mouse;
  CKey m_LastKey;

  std::map<std::string, std::map<int, float>> m_lastAxisMap;

  std::vector<CAction> m_queuedActions;
  CCriticalSection m_actionMutex;

  // Button translation
  std::unique_ptr<KODI::KEYMAP::IKeymapEnvironment> m_keymapEnvironment;
  std::unique_ptr<KODI::KEYMAP::CButtonTranslator> m_buttonTranslator;
  std::unique_ptr<KODI::KEYMAP::CCustomControllerTranslator> m_customControllerTranslator;
  std::unique_ptr<KODI::KEYMAP::CTouchTranslator> m_touchTranslator;
  std::unique_ptr<KODI::KEYMAP::CJoystickMapper> m_joystickTranslator;

  std::vector<KODI::KEYBOARD::IKeyboardDriverHandler*> m_keyboardHandlers;
  std::vector<KODI::MOUSE::IMouseDriverHandler*> m_mouseHandlers;

  std::unique_ptr<KODI::KEYBOARD::IKeyboardDriverHandler> m_keyboardEasterEgg;

  // Input state
  bool m_enableController = true;

  // Settings
  static const std::string SETTING_INPUT_ENABLE_CONTROLLER;
};

/// \}
