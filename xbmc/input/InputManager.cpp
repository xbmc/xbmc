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

#include <math.h>

#include "Application.h"
#include "InputManager.h"
#include "input/keyboard/IKeyboardHandler.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/Geometry.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIMessage.h"

#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif

#ifdef HAS_IRSERVERSUITE
#include "input/windows/IRServerSuite.h"
#endif

#include "ButtonTranslator.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/PeripheralImon.h"
#include "XBMC_vkeys.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "settings/Settings.h"

#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif

#include <algorithm>

#ifdef HAS_EVENT_SERVER
using EVENTSERVER::CEventServer;
#endif

using namespace KODI::MESSAGING;
using PERIPHERALS::CPeripherals;

CInputManager& CInputManager::GetInstance()
{
  static CInputManager inputManager;
  return inputManager;
}

void CInputManager::InitializeInputs()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  m_RemoteControl.Initialize();
#endif

  m_Keyboard.Initialize();

  m_Mouse.Initialize();
  m_Mouse.SetEnabled(CSettings::GetInstance().GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE));
}

void CInputManager::SetEnabledJoystick(bool enabled /* = true */)
{
  //! @todo
}

bool CInputManager::ProcessRemote(int windowId)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  if (m_RemoteControl.GetButton())
  {
    CKey key(m_RemoteControl.GetButton(), m_RemoteControl.GetHoldTime());
    m_RemoteControl.Reset();
    return OnKey(key);
  }
#endif
  return false;
}

bool CInputManager::ProcessPeripherals(float frameTime)
{
  CKey key;
  if (g_peripherals.GetNextKeypress(frameTime, key))
    return OnKey(key);
  return false;
}

bool CInputManager::ProcessMouse(int windowId)
{
  MEASURE_FUNCTION;

  if (!m_Mouse.IsActive() || !g_application.IsAppFocused())
    return false;

  // Get the mouse command ID
  uint32_t mousekey = m_Mouse.GetKey();
  if (mousekey == KEY_MOUSE_NOOP)
    return true;

  // Reset the screensaver and idle timers
  g_application.ResetSystemIdleTimer();
  g_application.ResetScreenSaver();

  if (g_application.WakeUpScreenSaverAndDPMS())
    return true;

  // Retrieve the corresponding action
  CKey key(mousekey, (unsigned int)0);
  CAction mouseaction = CButtonTranslator::GetInstance().GetAction(windowId, key);

  // Deactivate mouse if non-mouse action
  if (!mouseaction.IsMouse())
    m_Mouse.SetActive(false);

  // Consume ACTION_NOOP.
  // Some views or dialogs gets closed after any ACTION and
  // a sensitive mouse might cause problems.
  if (mouseaction.GetID() == ACTION_NOOP)
    return false;

  // If we couldn't find an action return false to indicate we have not
  // handled this mouse action
  if (!mouseaction.GetID())
  {
    CLog::LogF(LOGDEBUG, "unknown mouse command %d", mousekey);
    return false;
  }

  // Log mouse actions except for move and noop
  if (mouseaction.GetID() != ACTION_MOUSE_MOVE && mouseaction.GetID() != ACTION_NOOP)
    CLog::LogF(LOGDEBUG, "trying mouse action %s", mouseaction.GetName().c_str());

  // The action might not be a mouse action. For example wheel moves might
  // be mapped to volume up/down in mouse.xml. In this case we do not want
  // the mouse position saved in the action.
  if (!mouseaction.IsMouse())
    return g_application.OnAction(mouseaction);

  // This is a mouse action so we need to record the mouse position
  return g_application.OnAction(CAction(mouseaction.GetID(),
    m_Mouse.GetHold(MOUSE_LEFT_BUTTON),
    (float)m_Mouse.GetX(),
    (float)m_Mouse.GetY(),
    (float)m_Mouse.GetDX(),
    (float)m_Mouse.GetDY(),
    mouseaction.GetName()));
}

bool CInputManager::ProcessEventServer(int windowId, float frameTime)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    g_application.ResetSystemIdleTimer();
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();
  }

  // now handle any buttons or axis
  std::string strMapName;
  bool isAxis = false;
  float fAmount = 0.0;
  bool isJoystick = false;

  // es->ExecuteNextAction() invalidates the ref to the CEventServer instance
  // when the action exits XBMC
  es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;
  unsigned int wKeyID = es->GetButtonCode(strMapName, isAxis, fAmount, isJoystick);

  if (wKeyID)
  {
    if (strMapName.length() > 0)
    {
      // joysticks are not supported via eventserver
      if (isJoystick)
      {
        return false;
      }
      else // it is a customcontroller
      {
        int actionID;
        std::string actionName;
        
        // Translate using custom controller translator.
        if (CButtonTranslator::GetInstance().TranslateCustomControllerString(windowId, strMapName, wKeyID, actionID, actionName))
        {
          // break screensaver
          g_application.ResetSystemIdleTimer();
          g_application.ResetScreenSaver();
          
          // in case we wokeup the screensaver or screen - eat that action...
          if (g_application.WakeUpScreenSaverAndDPMS())
            return true;
          
          m_Mouse.SetActive(false);
          
          return ExecuteInputAction(CAction(actionID, fAmount, 0.0f, actionName));
        }
        else
        {
          CLog::Log(LOGDEBUG, "ERROR mapping customcontroller action. CustomController: %s %i", strMapName.c_str(), wKeyID);
        }
      }
    }
    else
    {
      CKey key;
      if (wKeyID & ES_FLAG_UNICODE)
      {
        key = CKey((uint8_t)0, wKeyID & ~ES_FLAG_UNICODE, 0, 0, 0);
        return OnKey(key);
      }

      if (wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, (BYTE)(255 * fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, (BYTE)(255 * fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, fAmount, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, fAmount, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, fAmount, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, fAmount, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      key.SetFromService(true);
      return OnKey(key);
    }
  }

  {
    CPoint pos;
    if (es->GetMousePos(pos.x, pos.y) && m_Mouse.IsEnabled())
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.xrel = 0;
      newEvent.motion.yrel = 0;
      newEvent.motion.state = 0;
      newEvent.motion.which = 0x10;  // just a different value to distinguish between mouse and event client device.
      newEvent.motion.x = (uint16_t)pos.x;
      newEvent.motion.y = (uint16_t)pos.y;
      g_application.OnEvent(newEvent);  // had to call this to update g_Mouse position
      return g_application.OnAction(CAction(ACTION_MOUSE_MOVE, pos.x, pos.y));
    }
  }
#endif
  return false;
}

void CInputManager::ProcessQueuedActions()
{
  std::vector<CAction> queuedActions;
  {
    CSingleLock lock(m_actionMutex);
    queuedActions.swap(m_queuedActions);
  }

  for (const CAction& action : queuedActions)
    g_application.OnAction(action);
}

void CInputManager::QueueAction(const CAction& action)
{
  CSingleLock lock(m_actionMutex);

  // Avoid dispatching multiple analog actions per frame with the same ID
  if (action.IsAnalog())
  {
    m_queuedActions.erase(std::remove_if(m_queuedActions.begin(), m_queuedActions.end(),
      [&action](const CAction& queuedAction)
      {
        return action.GetID() == queuedAction.GetID();
      }), m_queuedActions.end());
  }

  m_queuedActions.push_back(action);
}

bool CInputManager::Process(int windowId, float frameTime)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  // Read the input from a remote
  m_RemoteControl.Update();
#endif

  // process input actions
  ProcessRemote(windowId);
  ProcessEventServer(windowId, frameTime);
  ProcessPeripherals(frameTime);
  ProcessQueuedActions();
  
  return true;
}

bool CInputManager::OnEvent(XBMC_Event& newEvent)
{
  switch (newEvent.type)
  {
  case XBMC_KEYDOWN:
  {
    m_Keyboard.ProcessKeyDown(newEvent.key.keysym);
    CKey key = m_Keyboard.TranslateKey(newEvent.key.keysym);
    if (key.GetButtonCode() == m_LastKey.GetButtonCode() && m_LastKey.GetButtonCode() & CKey::MODIFIER_LONG)
    {
      // Do not repeat long presses
      break;
    }
    if (!CButtonTranslator::GetInstance().HasLonpressMapping(g_windowManager.GetActiveWindowID(), key))
    {
      m_LastKey.Reset();
      OnKey(key);
    }
    else
    {
      if (key.GetButtonCode() != m_LastKey.GetButtonCode() && key.GetButtonCode() & CKey::MODIFIER_LONG)
      {
        m_LastKey = key;  // OnKey is reentrant; need to do this before entering
        OnKey(key);
      }
      m_LastKey = key;
    }
    break;
  }
  case XBMC_KEYUP:
    m_Keyboard.ProcessKeyUp();
    if (m_LastKey.GetButtonCode() != KEY_INVALID && !(m_LastKey.GetButtonCode() & CKey::MODIFIER_LONG))
    {
      CKey key = m_LastKey;
      m_LastKey.Reset();  // OnKey is reentrant; need to do this before entering
      OnKey(key);
    }
    else
      m_LastKey.Reset();
    OnKeyUp(m_Keyboard.TranslateKey(newEvent.key.keysym));
    break;
  case XBMC_MOUSEBUTTONDOWN:
  case XBMC_MOUSEBUTTONUP:
  case XBMC_MOUSEMOTION:
    m_Mouse.HandleEvent(newEvent);
    ProcessMouse(g_windowManager.GetActiveWindowID());
    break;
  case XBMC_TOUCH:
  {
    if (newEvent.touch.action == ACTION_TOUCH_TAP)
    { // Send a mouse motion event with no dx,dy for getting the current guiitem selected
      g_application.OnAction(CAction(ACTION_MOUSE_MOVE, 0, newEvent.touch.x, newEvent.touch.y, 0, 0));
    }
    int actionId = 0;
    std::string actionString;
    if (newEvent.touch.action == ACTION_GESTURE_BEGIN || newEvent.touch.action == ACTION_GESTURE_END)
      actionId = newEvent.touch.action;
    else
    {
      int iWin = g_windowManager.GetActiveWindowID();
      CButtonTranslator::GetInstance().TranslateTouchAction(iWin, newEvent.touch.action, newEvent.touch.pointers, actionId, actionString);
    }

    if (actionId <= 0)
      return false;

    if ((actionId >= ACTION_TOUCH_TAP && actionId <= ACTION_GESTURE_END)
        || (actionId >= ACTION_MOUSE_START && actionId <= ACTION_MOUSE_END))
    {
      auto action = new CAction(actionId, 0, newEvent.touch.x, newEvent.touch.y, newEvent.touch.x2, newEvent.touch.y2);
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(action));
    }
    else
    {
      if (actionId == ACTION_BUILT_IN_FUNCTION && !actionString.empty())
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(actionId, actionString)));
      else
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(actionId)));
    }

    // Post an unfocus message for touch device after the action.
    if (newEvent.touch.action == ACTION_GESTURE_END || newEvent.touch.action == ACTION_TOUCH_TAP)
    {
      CGUIMessage msg(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
      CApplicationMessenger::GetInstance().SendGUIMessage(msg);
    }
    break;
  } //case
  }//switch

  return true;
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CInputManager::OnKey(const CKey& key)
{
  for (std::vector<KEYBOARD::IKeyboardHandler*>::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
  {
    if ((*it)->OnKeyPress(key))
      return true;
  }

  // Turn the mouse off, as we've just got a keypress from controller or remote
  m_Mouse.SetActive(false);

  // get the current active window
  int iWin = g_windowManager.GetActiveWindowID();

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CAction action = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // a key has been pressed.
  // reset Idle Timer
  g_application.ResetSystemIdleTimer();
  bool processKey = AlwaysProcess(action);

  if (StringUtils::StartsWithNoCase(action.GetName(), "CECToggleState") || StringUtils::StartsWithNoCase(action.GetName(), "CECStandby"))
  {
    // do not wake up the screensaver right after switching off the playing device
    if (StringUtils::StartsWithNoCase(action.GetName(), "CECToggleState"))
    {
      CLog::LogF(LOGDEBUG, "action %s [%d], toggling state of playing device", action.GetName().c_str(), action.GetID());
      bool result;
      CApplicationMessenger::GetInstance().SendMsg(TMSG_CECTOGGLESTATE, 0, 0, static_cast<void*>(&result));
      if (!result)
        return true;
    }
    else
    {
      CApplicationMessenger::GetInstance().PostMsg(TMSG_CECSTANDBY);
      return true;
    }
  }

  g_application.ResetScreenSaver();

  // allow some keys to be processed while the screensaver is active
  if (g_application.WakeUpScreenSaverAndDPMS(processKey) && !processKey)
  {
    CLog::LogF(LOGDEBUG, "%s pressed, screen saver/dpms woken up", m_Keyboard.GetKeyName((int)key.GetButtonCode()).c_str());
    return true;
  }

  if (iWin != WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard = key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow *window = g_windowManager.GetWindow(iWin);
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
      {
        // If this is an edit control set usekeyboard to true. This causes the
        // keypress to be processed directly not through the key mappings.
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT)
          useKeyboard = true;

        // If the key pressed is shift-A to shift-Z set usekeyboard to true.
        // This causes the keypress to be used for list navigation.
        if (control->IsContainer() && key.GetModifiers() == CKey::MODIFIER_SHIFT && key.GetVKey() >= XBMCVK_A && key.GetVKey() <= XBMCVK_Z)
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      // use the virtualkeyboard section of the keymap, and send keyboard-specific or navigation
      // actions through if that's what they are
      CAction action = CButtonTranslator::GetInstance().GetAction(WINDOW_DIALOG_KEYBOARD, key);
      if (!(action.GetID() == ACTION_MOVE_LEFT ||
        action.GetID() == ACTION_MOVE_RIGHT ||
        action.GetID() == ACTION_MOVE_UP ||
        action.GetID() == ACTION_MOVE_DOWN ||
        action.GetID() == ACTION_SELECT_ITEM ||
        action.GetID() == ACTION_ENTER ||
        action.GetID() == ACTION_PREVIOUS_MENU ||
        action.GetID() == ACTION_NAV_BACK))
      {
        // the action isn't plain navigation - check for a keyboard-specific keymap
        action = CButtonTranslator::GetInstance().GetAction(WINDOW_DIALOG_KEYBOARD, key, false);
        if (!(action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9) ||
            action.GetID() == ACTION_BACKSPACE ||
            action.GetID() == ACTION_SHIFT ||
            action.GetID() == ACTION_SYMBOLS ||
            action.GetID() == ACTION_CURSOR_LEFT ||
            action.GetID() == ACTION_CURSOR_RIGHT)
            action = CAction(0); // don't bother with this action
      }
      // else pass the keys through directly
      if (!action.GetID())
      {
        if (key.GetFromService())
          action = CAction(key.GetButtonCode() != KEY_INVALID ? key.GetButtonCode() : 0, key.GetUnicode());
        else
        {
          // Check for paste keypress
#ifdef TARGET_WINDOWS
          // In Windows paste is ctrl-V
          if (key.GetVKey() == XBMCVK_V && key.GetModifiers() == CKey::MODIFIER_CTRL)
#elif defined(TARGET_LINUX)
          // In Linux paste is ctrl-V
          if (key.GetVKey() == XBMCVK_V && key.GetModifiers() == CKey::MODIFIER_CTRL)
#elif defined(TARGET_DARWIN_OSX)
          // In OSX paste is cmd-V
          if (key.GetVKey() == XBMCVK_V && key.GetModifiers() == CKey::MODIFIER_META)
#else
          // Placeholder for other operating systems
          if (false)
#endif
            action = CAction(ACTION_PASTE);
          // If the unicode is non-zero the keypress is a non-printing character
          else if (key.GetUnicode())
            action = CAction(key.GetAscii() | KEY_ASCII, key.GetUnicode());
          // The keypress is a non-printing character
          else
            action = CAction(key.GetVKey() | KEY_VKEY);
        }
      }

      CLog::LogF(LOGDEBUG, "%s pressed, trying keyboard action %x", m_Keyboard.GetKeyName((int)key.GetButtonCode()).c_str(), action.GetID());

      if (g_application.OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromService())
    {
      if (key.GetButtonCode() != KEY_INVALID)
        action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
    else
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
  }
  if (!key.IsAnalogButton())
    CLog::LogF(LOGDEBUG, "%s pressed, action is %s", m_Keyboard.GetKeyName((int)key.GetButtonCode()).c_str(), action.GetName().c_str());

  return ExecuteInputAction(action);
}

void CInputManager::OnKeyUp(const CKey& key)
{
  for (std::vector<KEYBOARD::IKeyboardHandler*>::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
    (*it)->OnKeyRelease(key);
}

bool CInputManager::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (!action.GetName().empty())
  {
    std::string builtInFunction;
    std::vector<std::string> params;
    CUtil::SplitExecFunction(action.GetName(), builtInFunction, params);
    StringUtils::ToLower(builtInFunction);

    // should this button be handled normally or just cancel the screensaver?
    if (builtInFunction == "powerdown"
        || builtInFunction == "reboot"
        || builtInFunction == "restart"
        || builtInFunction == "restartapp"
        || builtInFunction == "suspend"
        || builtInFunction == "hibernate"
        || builtInFunction == "quit"
        || builtInFunction == "shutdown")
    {
      return true;
    }
  }

  return false;
}

bool CInputManager::ExecuteInputAction(const CAction &action)
{
  bool bResult = false;

  // play sound before the action unless the button is held,
  // where we execute after the action as held actions aren't fired every time.
  if (action.GetHoldTime())
  {
    bResult = g_application.OnAction(action);
    if (bResult)
      g_audioManager.PlayActionSound(action);
  }
  else
  {
    g_audioManager.PlayActionSound(action);
    bResult = g_application.OnAction(action);
  }
  return bResult;
}

bool CInputManager::HasBuiltin(const std::string& command)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  return command == "lirc.stop"  ||
         command ==" lirc.start" ||
         command == "lirc.send";
#endif

  return false;
}

int CInputManager::ExecuteBuiltin(const std::string& execute, const std::vector<std::string>& params)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  if (execute == "lirc.stop")
  {
    m_RemoteControl.Disconnect();
    m_RemoteControl.SetEnabled(false);
  }
  else if (execute == "lirc.start")
  {
    m_RemoteControl.SetEnabled(true);
    m_RemoteControl.Initialize();
  }
  else if (execute == "lirc.send")
  {
    std::string command;
    for (int i = 0; i < (int)params.size(); i++)
    {
      command += params[i];
      if (i < (int)params.size() - 1)
        command += ' ';
    }
    m_RemoteControl.AddSendCommand(command);
  }
  else
    return -1;
#endif
  return 0;
}

void CInputManager::SetMouseActive(bool active /* = true */)
{
  m_Mouse.SetActive(active);
}

void CInputManager::SetMouseEnabled(bool mouseEnabled /* = true */)
{
  m_Mouse.SetEnabled(mouseEnabled);
}

bool CInputManager::IsMouseActive()
{
  return m_Mouse.IsActive();
}

MOUSE_STATE CInputManager::GetMouseState()
{
  return m_Mouse.GetState();
}

MousePosition CInputManager::GetMousePosition()
{
  return m_Mouse.GetPosition();
}

void CInputManager::SetMouseResolution(int maxX, int maxY, float speedX, float speedY)
{
  m_Mouse.SetResolution(maxX, maxY, speedX, speedY);
}

void CInputManager::SetMouseState(MOUSE_STATE mouseState)
{
  m_Mouse.SetState(mouseState);
}

bool CInputManager::IsRemoteControlEnabled()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  return m_RemoteControl.IsInUse();
#else
  return false;
#endif
}

bool CInputManager::IsRemoteControlInitialized()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  return m_RemoteControl.IsInitialized();
#else
  return false;
#endif
}

void CInputManager::EnableRemoteControl()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  m_RemoteControl.SetEnabled(true);
  if (!m_RemoteControl.IsInitialized())
  {
    m_RemoteControl.Initialize();
  }
#endif
}

void CInputManager::DisableRemoteControl()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  m_RemoteControl.Disconnect();
  m_RemoteControl.SetEnabled(false);
#endif
}

void CInputManager::InitializeRemoteControl()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  if (!m_RemoteControl.IsInitialized())
    m_RemoteControl.Initialize();
#endif
}

void CInputManager::SetRemoteControlName(const std::string& name)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  m_RemoteControl.SetDeviceName(name);
#endif
}

void CInputManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_INPUT_ENABLEMOUSE)
    m_Mouse.SetEnabled(dynamic_cast<const CSettingBool*>(setting)->GetValue());
}

void CInputManager::RegisterKeyboardHandler(KEYBOARD::IKeyboardHandler* handler)
{
  if (std::find(m_keyboardHandlers.begin(), m_keyboardHandlers.end(), handler) == m_keyboardHandlers.end())
    m_keyboardHandlers.push_back(handler);
}

void CInputManager::UnregisterKeyboardHandler(KEYBOARD::IKeyboardHandler* handler)
{
  m_keyboardHandlers.erase(std::remove(m_keyboardHandlers.begin(), m_keyboardHandlers.end(), handler), m_keyboardHandlers.end());
}
