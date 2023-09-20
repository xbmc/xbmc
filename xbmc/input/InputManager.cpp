/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputManager.h"

#include "ButtonTranslator.h"
#include "CustomControllerTranslator.h"
#include "JoystickMapper.h"
#include "KeymapEnvironment.h"
#include "ServiceBroker.h"
#include "TouchTranslator.h"
#include "XBMC_vkeys.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "input/keyboard/KeyboardEasterEgg.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/MouseTranslator.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "messaging/ApplicationMessenger.h"
#include "network/EventServer.h"
#include "peripherals/Peripherals.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/ExecString.h"
#include "utils/Geometry.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <math.h>
#include <mutex>

using EVENTSERVER::CEventServer;

using namespace KODI;

const std::string CInputManager::SETTING_INPUT_ENABLE_CONTROLLER = "input.enablejoystick";

CInputManager::CInputManager()
  : m_keymapEnvironment(new CKeymapEnvironment),
    m_buttonTranslator(new CButtonTranslator),
    m_customControllerTranslator(new CCustomControllerTranslator),
    m_touchTranslator(new CTouchTranslator),
    m_joystickTranslator(new CJoystickMapper),
    m_keyboardEasterEgg(new KEYBOARD::CKeyboardEasterEgg)
{
  m_buttonTranslator->RegisterMapper("touch", m_touchTranslator.get());
  m_buttonTranslator->RegisterMapper("customcontroller", m_customControllerTranslator.get());
  m_buttonTranslator->RegisterMapper("joystick", m_joystickTranslator.get());

  RegisterKeyboardDriverHandler(m_keyboardEasterEgg.get());

  // Register settings
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_INPUT_ENABLEMOUSE);
  settingSet.insert(SETTING_INPUT_ENABLE_CONTROLLER);
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, settingSet);
}

CInputManager::~CInputManager()
{
  Deinitialize();

  // Unregister settings
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);

  UnregisterKeyboardDriverHandler(m_keyboardEasterEgg.get());

  m_buttonTranslator->UnregisterMapper(m_touchTranslator.get());
  m_buttonTranslator->UnregisterMapper(m_customControllerTranslator.get());
  m_buttonTranslator->UnregisterMapper(m_joystickTranslator.get());
}

void CInputManager::InitializeInputs()
{
  m_Keyboard.Initialize();

  m_Mouse.Initialize();
  m_Mouse.SetEnabled(CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_INPUT_ENABLEMOUSE));

  m_enableController = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_INPUT_ENABLE_CONTROLLER);
}

void CInputManager::Deinitialize()
{
}

bool CInputManager::ProcessPeripherals(float frameTime)
{
  CKey key;
  if (CServiceBroker::GetPeripherals().GetNextKeypress(frameTime, key))
    return OnKey(key);
  return false;
}

bool CInputManager::ProcessMouse(int windowId)
{
  if (!m_Mouse.IsActive() || !g_application.IsAppFocused())
    return false;

  // Get the mouse command ID
  uint32_t mousekey = m_Mouse.GetKey();
  if (mousekey == KEY_MOUSE_NOOP)
    return true;

  // Reset the screensaver and idle timers
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetSystemIdleTimer();
  appPower->ResetScreenSaver();

  if (appPower->WakeUpScreenSaverAndDPMS())
    return true;

  // Retrieve the corresponding action
  CKey key(mousekey, (unsigned int)0);
  CAction mouseaction = m_buttonTranslator->GetAction(windowId, key);

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
    CLog::LogF(LOGDEBUG, "unknown mouse command {}", mousekey);
    return false;
  }

  // Log mouse actions except for move and noop
  if (mouseaction.GetID() != ACTION_MOUSE_MOVE && mouseaction.GetID() != ACTION_NOOP)
    CLog::LogF(LOGDEBUG, "trying mouse action {}", mouseaction.GetName());

  // The action might not be a mouse action. For example wheel moves might
  // be mapped to volume up/down in mouse.xml. In this case we do not want
  // the mouse position saved in the action.
  if (!mouseaction.IsMouse())
    return g_application.OnAction(mouseaction);

  // This is a mouse action so we need to record the mouse position
  return g_application.OnAction(
      CAction(mouseaction.GetID(), static_cast<uint32_t>(m_Mouse.GetHold(MOUSE_LEFT_BUTTON)),
              static_cast<float>(m_Mouse.GetX()), static_cast<float>(m_Mouse.GetY()),
              static_cast<float>(m_Mouse.GetDX()), static_cast<float>(m_Mouse.GetDY()), 0.0f, 0.0f,
              mouseaction.GetName()));
}

bool CInputManager::ProcessEventServer(int windowId, float frameTime)
{
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->ResetSystemIdleTimer();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();
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
        if (m_customControllerTranslator->TranslateCustomControllerString(
                windowId, strMapName, wKeyID, actionID, actionName))
        {
          // break screensaver
          auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          appPower->ResetSystemIdleTimer();
          appPower->ResetScreenSaver();

          // in case we wokeup the screensaver or screen - eat that action...
          if (appPower->WakeUpScreenSaverAndDPMS())
            return true;

          m_Mouse.SetActive(false);

          CLog::Log(LOGDEBUG, "EventServer: key {} translated to action {}", wKeyID, actionName);

          return ExecuteInputAction(CAction(actionID, fAmount, 0.0f, actionName, 0, wKeyID));
        }
        else
        {
          CLog::Log(LOGDEBUG, "ERROR mapping customcontroller action. CustomController: {} {}",
                    strMapName, wKeyID);
        }
      }
    }
    else
    {
      CKey key;
      if (wKeyID & ES_FLAG_UNICODE)
      {
        key = CKey(0u, 0u, static_cast<wchar_t>(wKeyID & ~ES_FLAG_UNICODE), 0, 0, 0, 0);
        return OnKey(key);
      }

      if (wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, static_cast<uint8_t>(255 * fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, static_cast<uint8_t>(255 * fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
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
      XBMC_Event newEvent = {};
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.x = (uint16_t)pos.x;
      newEvent.motion.y = (uint16_t)pos.y;
      CServiceBroker::GetAppPort()->OnEvent(
          newEvent); // had to call this to update g_Mouse position
      return g_application.OnAction(CAction(ACTION_MOUSE_MOVE, pos.x, pos.y));
    }
  }

  return false;
}

void CInputManager::ProcessQueuedActions()
{
  std::vector<CAction> queuedActions;
  {
    std::unique_lock<CCriticalSection> lock(m_actionMutex);
    queuedActions.swap(m_queuedActions);
  }

  for (const CAction& action : queuedActions)
    g_application.OnAction(action);
}

void CInputManager::QueueAction(const CAction& action)
{
  std::unique_lock<CCriticalSection> lock(m_actionMutex);

  // Avoid dispatching multiple analog actions per frame with the same ID
  if (action.IsAnalog())
  {
    m_queuedActions.erase(std::remove_if(m_queuedActions.begin(), m_queuedActions.end(),
                                         [&action](const CAction& queuedAction) {
                                           return action.GetID() == queuedAction.GetID();
                                         }),
                          m_queuedActions.end());
  }

  m_queuedActions.push_back(action);
}

bool CInputManager::Process(int windowId, float frameTime)
{
  // process input actions
  ProcessEventServer(windowId, frameTime);
  ProcessPeripherals(frameTime);
  ProcessQueuedActions();

  // Inform the environment of the new active window ID
  m_keymapEnvironment->SetWindowID(windowId);

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
      OnKey(key);
      break;
    }
    case XBMC_KEYUP:
      m_Keyboard.ProcessKeyUp();
      OnKeyUp(m_Keyboard.TranslateKey(newEvent.key.keysym));
      break;
    case XBMC_MOUSEBUTTONDOWN:
    case XBMC_MOUSEBUTTONUP:
    case XBMC_MOUSEMOTION:
    {
      bool handled = false;

      for (auto driverHandler : m_mouseHandlers)
      {
        switch (newEvent.type)
        {
          case XBMC_MOUSEMOTION:
          {
            if (driverHandler->OnPosition(newEvent.motion.x, newEvent.motion.y))
              handled = true;
            break;
          }
          case XBMC_MOUSEBUTTONDOWN:
          {
            MOUSE::BUTTON_ID buttonId;
            if (CMouseTranslator::TranslateEventID(newEvent.button.button, buttonId))
            {
              if (driverHandler->OnButtonPress(buttonId))
                handled = true;
            }
            break;
          }
          case XBMC_MOUSEBUTTONUP:
          {
            MOUSE::BUTTON_ID buttonId;
            if (CMouseTranslator::TranslateEventID(newEvent.button.button, buttonId))
              driverHandler->OnButtonRelease(buttonId);
            break;
          }
          default:
            break;
        }

        if (handled)
          break;
      }

      if (!handled)
      {
        m_Mouse.HandleEvent(newEvent);
        ProcessMouse(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      }
      break;
    }
    case XBMC_TOUCH:
    {
      if (newEvent.touch.action == ACTION_TOUCH_TAP)
      { // Send a mouse motion event with no dx,dy for getting the current guiitem selected
        g_application.OnAction(
            CAction(ACTION_MOUSE_MOVE, 0, newEvent.touch.x, newEvent.touch.y, 0, 0));
      }
      int actionId = 0;
      std::string actionString;
      if (newEvent.touch.action == ACTION_GESTURE_BEGIN ||
          newEvent.touch.action == ACTION_GESTURE_END ||
          newEvent.touch.action == ACTION_GESTURE_ABORT)
        actionId = newEvent.touch.action;
      else
      {
        int iWin = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();
        m_touchTranslator->TranslateTouchAction(iWin, newEvent.touch.action,
                                                newEvent.touch.pointers, actionId, actionString);
      }

      if (actionId <= 0)
        return false;

      if ((actionId >= ACTION_TOUCH_TAP && actionId <= ACTION_GESTURE_END) ||
          (actionId >= ACTION_MOUSE_START && actionId <= ACTION_MOUSE_END))
      {
        auto action =
            new CAction(actionId, 0, newEvent.touch.x, newEvent.touch.y, newEvent.touch.x2,
                        newEvent.touch.y2, newEvent.touch.x3, newEvent.touch.y3);
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                   static_cast<void*>(action));
      }
      else
      {
        if (actionId == ACTION_BUILT_IN_FUNCTION && !actionString.empty())
          CServiceBroker::GetAppMessenger()->PostMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(actionId, actionString)));
        else
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                     static_cast<void*>(new CAction(actionId)));
      }

      break;
    } // case
    case XBMC_BUTTON:
    {
      HandleKey(
          m_buttonStat.TranslateKey(CKey(newEvent.keybutton.button, newEvent.keybutton.holdtime)));
      break;
    }
  } // switch

  return true;
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CInputManager::OnKey(const CKey& key)
{
  bool bHandled = false;

  for (auto handler : m_keyboardHandlers)
  {
    if (handler->OnKeyPress(key))
    {
      bHandled = true;
      break;
    }
  }

  if (bHandled)
  {
    m_LastKey.Reset();
  }
  else
  {
    if (key.GetButtonCode() == m_LastKey.GetButtonCode() &&
        (m_LastKey.GetButtonCode() & CKey::MODIFIER_LONG))
    {
      // Do not repeat long presses
    }
    else
    {
      // Event server keyboard doesn't give normal key up and key down, so don't
      // process for long press if that is the source
      if (key.GetFromService() ||
          !m_buttonTranslator->HasLongpressMapping(
              CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), key))
      {
        m_LastKey.Reset();
        bHandled = HandleKey(key);
      }
      else
      {
        if (key.GetButtonCode() != m_LastKey.GetButtonCode() &&
            (key.GetButtonCode() & CKey::MODIFIER_LONG))
        {
          m_LastKey = key; // OnKey is reentrant; need to do this before entering
          bHandled = HandleKey(key);
        }

        m_LastKey = key;
      }
    }
  }

  return bHandled;
}

bool CInputManager::HandleKey(const CKey& key)
{
  // Turn the mouse off, as we've just got a keypress from controller or remote
  m_Mouse.SetActive(false);

  // get the current active window
  int iWin = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CAction action = m_buttonTranslator->GetAction(iWin, key);

  // a key has been pressed.
  // reset Idle Timer
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetSystemIdleTimer();
  bool processKey = AlwaysProcess(action);

  if (StringUtils::StartsWithNoCase(action.GetName(), "CECToggleState") ||
      StringUtils::StartsWithNoCase(action.GetName(), "CECStandby"))
  {
    // do not wake up the screensaver right after switching off the playing device
    if (StringUtils::StartsWithNoCase(action.GetName(), "CECToggleState"))
    {
      CLog::LogF(LOGDEBUG, "action {} [{}], toggling state of playing device", action.GetName(),
                 action.GetID());
      bool result;
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_CECTOGGLESTATE, 0, 0,
                                                 static_cast<void*>(&result));
      if (!result)
        return true;
    }
    else
    {
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CECSTANDBY);
      return true;
    }
  }

  appPower->ResetScreenSaver();

  // allow some keys to be processed while the screensaver is active
  if (appPower->WakeUpScreenSaverAndDPMS(processKey) && !processKey)
  {
    CLog::LogF(LOGDEBUG, "{} pressed, screen saver/dpms woken up",
               m_Keyboard.GetKeyName((int)key.GetButtonCode()));
    return true;
  }

  if (iWin != WINDOW_FULLSCREEN_VIDEO && iWin != WINDOW_FULLSCREEN_GAME)
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard =
        key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(iWin);
    if (window)
    {
      CGUIControl* control = window->GetFocusedControl();
      if (control)
      {
        // If this is an edit control set usekeyboard to true. This causes the
        // keypress to be processed directly not through the key mappings.
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT)
          useKeyboard = true;

        // If the key pressed is shift-A to shift-Z set usekeyboard to true.
        // This causes the keypress to be used for list navigation.
        if (control->IsContainer() && key.GetModifiers() == CKey::MODIFIER_SHIFT &&
            key.GetUnicode())
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      // use the virtualkeyboard section of the keymap, and send keyboard-specific or navigation
      // actions through if that's what they are
      CAction action = m_buttonTranslator->GetAction(WINDOW_DIALOG_KEYBOARD, key);
      if (!(action.GetID() == ACTION_MOVE_LEFT || action.GetID() == ACTION_MOVE_RIGHT ||
            action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN ||
            action.GetID() == ACTION_SELECT_ITEM || action.GetID() == ACTION_ENTER ||
            action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_NAV_BACK ||
            action.GetID() == ACTION_VOICE_RECOGNIZE))
      {
        // the action isn't plain navigation - check for a keyboard-specific keymap
        action = m_buttonTranslator->GetAction(WINDOW_DIALOG_KEYBOARD, key, false);
        if (!(action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9) ||
            action.GetID() == ACTION_BACKSPACE || action.GetID() == ACTION_SHIFT ||
            action.GetID() == ACTION_SYMBOLS || action.GetID() == ACTION_CURSOR_LEFT ||
            action.GetID() == ACTION_CURSOR_RIGHT)
          action = CAction(0); // don't bother with this action
      }
      // else pass the keys through directly
      if (!action.GetID())
      {
        if (key.GetFromService())
          action = CAction(key.GetButtonCode() != KEY_INVALID ? key.GetButtonCode() : 0,
                           key.GetUnicode());
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
            action = CAction(KEY_UNICODE, key.GetUnicode());
          // The keypress is a non-printing character
          else
            action = CAction(key.GetVKey() | KEY_VKEY);
        }
      }

      CLog::LogF(LOGDEBUG, "{} pressed, trying keyboard action {:x}",
                 m_Keyboard.GetKeyName((int)key.GetButtonCode()), action.GetID());

      if (g_application.OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromService())
    {
      if (key.GetButtonCode() != KEY_INVALID)
        action = m_buttonTranslator->GetAction(iWin, key);
    }
    else
      action = m_buttonTranslator->GetAction(iWin, key);
  }
  if (!key.IsAnalogButton())
    CLog::LogF(LOGDEBUG, "{} pressed, window {}, action is {}",
               m_Keyboard.GetKeyName((int)key.GetButtonCode()), iWin, action.GetName());

  return ExecuteInputAction(action);
}

void CInputManager::OnKeyUp(const CKey& key)
{
  for (auto handler : m_keyboardHandlers)
    handler->OnKeyRelease(key);

  if (m_LastKey.GetButtonCode() != KEY_INVALID &&
      !(m_LastKey.GetButtonCode() & CKey::MODIFIER_LONG))
  {
    CKey key = m_LastKey;
    m_LastKey.Reset(); // OnKey is reentrant; need to do this before entering
    HandleKey(key);
  }
  else
    m_LastKey.Reset();
}

bool CInputManager::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (!action.GetName().empty())
  {
    const CExecString exec(action.GetName());
    if (exec.IsValid())
    {
      const std::string builtInFunction = exec.GetFunction();

      // should this button be handled normally or just cancel the screensaver?
      if (builtInFunction == "powerdown" || builtInFunction == "reboot" ||
          builtInFunction == "restart" || builtInFunction == "restartapp" ||
          builtInFunction == "suspend" || builtInFunction == "hibernate" ||
          builtInFunction == "quit" || builtInFunction == "shutdown" ||
          builtInFunction == "volumeup" || builtInFunction == "volumedown" ||
          builtInFunction == "mute" || builtInFunction == "RunAppleScript" ||
          builtInFunction == "RunAddon" || builtInFunction == "RunPlugin" ||
          builtInFunction == "RunScript" || builtInFunction == "System.Exec" ||
          builtInFunction == "System.ExecWait")
      {
        return true;
      }
    }
  }

  return false;
}

bool CInputManager::ExecuteInputAction(const CAction& action)
{
  bool bResult = false;
  CGUIComponent* gui = CServiceBroker::GetGUI();

  // play sound before the action unless the button is held,
  // where we execute after the action as held actions aren't fired every time.
  if (action.GetHoldTime())
  {
    bResult = g_application.OnAction(action);
    if (bResult && gui)
      gui->GetAudioManager().PlayActionSound(action);
  }
  else
  {
    if (gui)
      gui->GetAudioManager().PlayActionSound(action);

    bResult = g_application.OnAction(action);
  }
  return bResult;
}

bool CInputManager::HasBuiltin(const std::string& command)
{
  return false;
}

int CInputManager::ExecuteBuiltin(const std::string& execute,
                                  const std::vector<std::string>& params)
{
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

bool CInputManager::IsControllerEnabled() const
{
  return m_enableController;
}

void CInputManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_INPUT_ENABLEMOUSE)
    m_Mouse.SetEnabled(std::dynamic_pointer_cast<const CSettingBool>(setting)->GetValue());

  else if (settingId == SETTING_INPUT_ENABLE_CONTROLLER)
    m_enableController = std::dynamic_pointer_cast<const CSettingBool>(setting)->GetValue();
}

bool CInputManager::OnAction(const CAction& action)
{
  if (action.GetID() != ACTION_NONE)
  {
    if (action.IsAnalog())
    {
      QueueAction(action);
    }
    else
    {
      // If button was pressed this frame, send action
      if (action.GetHoldTime() == 0)
      {
        QueueAction(action);
      }
      else
      {
        // Only send repeated actions for basic navigation commands
        bool bIsNavigation = false;

        switch (action.GetID())
        {
          case ACTION_MOVE_LEFT:
          case ACTION_MOVE_RIGHT:
          case ACTION_MOVE_UP:
          case ACTION_MOVE_DOWN:
          case ACTION_PAGE_UP:
          case ACTION_PAGE_DOWN:
            bIsNavigation = true;
            break;

          default:
            break;
        }

        if (bIsNavigation)
          QueueAction(action);
      }
    }

    return true;
  }

  return false;
}

bool CInputManager::LoadKeymaps()
{
  bool bSuccess = false;

  if (m_buttonTranslator->Load())
  {
    bSuccess = true;
  }

  SetChanged();
  NotifyObservers(ObservableMessageButtonMapsChanged);

  return bSuccess;
}

bool CInputManager::ReloadKeymaps()
{
  return LoadKeymaps();
}

void CInputManager::ClearKeymaps()
{
  m_buttonTranslator->Clear();

  SetChanged();
  NotifyObservers(ObservableMessageButtonMapsChanged);
}

void CInputManager::AddKeymap(const std::string& keymap)
{
  if (m_buttonTranslator->AddDevice(keymap))
  {
    SetChanged();
    NotifyObservers(ObservableMessageButtonMapsChanged);
  }
}

void CInputManager::RemoveKeymap(const std::string& keymap)
{
  if (m_buttonTranslator->RemoveDevice(keymap))
  {
    SetChanged();
    NotifyObservers(ObservableMessageButtonMapsChanged);
  }
}

CAction CInputManager::GetAction(int window, const CKey& key, bool fallback /* = true */)
{
  return m_buttonTranslator->GetAction(window, key, fallback);
}

bool CInputManager::TranslateCustomControllerString(int windowId,
                                                    const std::string& controllerName,
                                                    int buttonId,
                                                    int& action,
                                                    std::string& strAction)
{
  return m_customControllerTranslator->TranslateCustomControllerString(windowId, controllerName,
                                                                       buttonId, action, strAction);
}

bool CInputManager::TranslateTouchAction(
    int windowId, int touchAction, int touchPointers, int& action, std::string& actionString)
{
  return m_touchTranslator->TranslateTouchAction(windowId, touchAction, touchPointers, action,
                                                 actionString);
}

std::vector<std::shared_ptr<const IWindowKeymap>> CInputManager::GetJoystickKeymaps() const
{
  return m_joystickTranslator->GetJoystickKeymaps();
}

void CInputManager::RegisterKeyboardDriverHandler(KEYBOARD::IKeyboardDriverHandler* handler)
{
  if (std::find(m_keyboardHandlers.begin(), m_keyboardHandlers.end(), handler) ==
      m_keyboardHandlers.end())
    m_keyboardHandlers.insert(m_keyboardHandlers.begin(), handler);
}

void CInputManager::UnregisterKeyboardDriverHandler(KEYBOARD::IKeyboardDriverHandler* handler)
{
  m_keyboardHandlers.erase(
      std::remove(m_keyboardHandlers.begin(), m_keyboardHandlers.end(), handler),
      m_keyboardHandlers.end());
}

void CInputManager::RegisterMouseDriverHandler(MOUSE::IMouseDriverHandler* handler)
{
  if (std::find(m_mouseHandlers.begin(), m_mouseHandlers.end(), handler) == m_mouseHandlers.end())
    m_mouseHandlers.insert(m_mouseHandlers.begin(), handler);
}

void CInputManager::UnregisterMouseDriverHandler(MOUSE::IMouseDriverHandler* handler)
{
  m_mouseHandlers.erase(std::remove(m_mouseHandlers.begin(), m_mouseHandlers.end(), handler),
                        m_mouseHandlers.end());
}
