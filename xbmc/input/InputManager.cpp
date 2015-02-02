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
#include <math.h>

#include "Application.h"
#include "InputManager.h"
#include "guilib/Geometry.h"

#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif

#ifdef HAS_IRSERVERSUITE
#include "input/windows/IRServerSuite.h"
#endif

#if SDL_VERSION == 1
#include <SDL/SDL.h>
#elif SDL_VERSION == 2
#include <SDL2/SDL.h>
#endif

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "input/SDLJoystick.h"
#endif
#include "ButtonTranslator.h"
#include "guilib/Key.h"
#include "input/MouseStat.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif

#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif

using namespace PERIPHERALS;

CInputManager& CInputManager::GetInstance()
{
  static CInputManager inputManager;
  return inputManager;
}

void CInputManager::InitializeInputs()
{
#ifdef HAS_SDL_JOYSTICK
  // Pass the mapping of axis to triggers to m_Joystick
  m_Joystick.LoadAxesConfigs(CButtonTranslator::GetInstance().GetAxesConfigs());
#endif
}

void CInputManager::ReInitializeJoystick()
{
#ifdef HAS_SDL_JOYSTICK
  m_Joystick.Reinitialize();
#endif
}

void CInputManager::SetEnabledJoystick(bool enabled /* = true */)
{
#ifdef HAS_SDL_JOYSTICK
  m_Joystick.SetEnabled(enabled);
#endif
}

#if defined(HAS_SDL_JOYSTICK) && !defined(TARGET_WINDOWS)
void CInputManager::UpdateJoystick(SDL_Event& joyEvent)
{
  m_Joystick.Update(joyEvent);
}
#endif

bool CInputManager::ProcessGamepad(int windowId)
{
#ifdef HAS_SDL_JOYSTICK
  if (!g_application.IsAppFocused())
    return false;

  int keymapId, joyId;
  m_Joystick.Update();
  std::string joyName;
  if (m_Joystick.GetButton(joyName, joyId))
  {
    g_application.ResetSystemIdleTimer();

    g_application.ResetScreenSaver();
    if (g_application.WakeUpScreenSaverAndDPMS())
    {
      m_Joystick.Reset();
      return true;
    }

    int actionID;
    std::string actionName;
    bool fullrange;
    keymapId = joyId + 1;

    if (CButtonTranslator::GetInstance().TranslateJoystickString(windowId, joyName, keymapId, JACTIVE_BUTTON, actionID, actionName, fullrange))
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_Mouse.SetActive(false);
      return g_application.ExecuteInputAction(action);
    }
  }
  if (m_Joystick.GetAxis(joyName, joyId))
  {
    keymapId = joyId + 1;
    if (m_Joystick.GetAmount(joyName, joyId) < 0)
    {
      keymapId = -keymapId;
    }

    int actionID;
    std::string actionName;
    bool fullrange;
    if (CButtonTranslator::GetInstance().TranslateJoystickString(windowId, joyName, keymapId, JACTIVE_AXIS, actionID, actionName, fullrange))
    {
      g_application.ResetScreenSaver();
      if (g_application.WakeUpScreenSaverAndDPMS())
      {
        return true;
      }

      float amount = m_Joystick.GetAmount(joyName, joyId);
      CAction action(actionID, fullrange ? (amount + 1.0f) / 2.0f : fabs(amount), 0.0f, actionName);
      g_Mouse.SetActive(false);
      return g_application.ExecuteInputAction(action);
    }
  }
  int position = 0;
  if (m_Joystick.GetHat(joyName, joyId, position))
  {
    keymapId = joyId + 1;
    // reset Idle Timer
    g_application.ResetSystemIdleTimer();

    g_application.ResetScreenSaver();
    if (g_application.WakeUpScreenSaverAndDPMS())
    {
      m_Joystick.Reset();
      return true;
    }

    int actionID;
    std::string actionName;
    bool fullrange;

    keymapId = position << 16 | keymapId;

    if (keymapId && CButtonTranslator::GetInstance().TranslateJoystickString(windowId, joyName, keymapId, JACTIVE_HAT, actionID, actionName, fullrange))
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_Mouse.SetActive(false);
      return g_application.ExecuteInputAction(action);
    }
  }
#endif
  return false;
}

bool CInputManager::ProcessRemote(int windowId)
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  if (g_RemoteControl.GetButton())
  {
    CKey key(g_RemoteControl.GetButton(), g_RemoteControl.GetHoldTime());
    g_RemoteControl.Reset();
    return g_application.OnKey(key);
  }
#endif
  return false;
}

bool CInputManager::ProcessPeripherals(float frameTime)
{
  CKey key;
  if (g_peripherals.GetNextKeypress(frameTime, key))
    return g_application.OnKey(key);
  return false;
}

bool CInputManager::ProcessMouse(int windowId)
{
  MEASURE_FUNCTION;

  if (!g_Mouse.IsActive() || !g_application.IsAppFocused())
    return false;

  // Get the mouse command ID
  uint32_t mousekey = g_Mouse.GetKey();
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
    g_Mouse.SetActive(false);

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
    g_Mouse.GetHold(MOUSE_LEFT_BUTTON),
    (float)g_Mouse.GetX(),
    (float)g_Mouse.GetY(),
    (float)g_Mouse.GetDX(),
    (float)g_Mouse.GetDY(),
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
  std::string joystickName;
  bool isAxis = false;
  float fAmount = 0.0;

  // es->ExecuteNextAction() invalidates the ref to the CEventServer instance
  // when the action exits XBMC
  es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;
  unsigned int wKeyID = es->GetButtonCode(joystickName, isAxis, fAmount);

  if (wKeyID)
  {
    if (joystickName.length() > 0)
    {
      if (isAxis == true)
      {
        if (fabs(fAmount) >= 0.08)
          m_lastAxisMap[joystickName][wKeyID] = fAmount;
        else
          m_lastAxisMap[joystickName].erase(wKeyID);
      }

      return ProcessJoystickEvent(windowId, joystickName, wKeyID, isAxis ? JACTIVE_AXIS : JACTIVE_BUTTON, fAmount);
    }
    else
    {
      CKey key;
      if (wKeyID & ES_FLAG_UNICODE)
      {
        key = CKey((uint8_t)0, wKeyID & ~ES_FLAG_UNICODE, 0, 0, 0);
        return g_application.OnKey(key);
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
      return g_application.OnKey(key);
    }
  }

  if (!m_lastAxisMap.empty())
  {
    // Process all the stored axis.
    for (std::map<std::string, std::map<int, float> >::iterator iter = m_lastAxisMap.begin(); iter != m_lastAxisMap.end(); ++iter)
    {
      for (std::map<int, float>::iterator iterAxis = (*iter).second.begin(); iterAxis != (*iter).second.end(); ++iterAxis)
        ProcessJoystickEvent(windowId, (*iter).first, (*iterAxis).first, JACTIVE_AXIS, (*iterAxis).second);
    }
  }

  {
    CPoint pos;
    if (es->GetMousePos(pos.x, pos.y) && g_Mouse.IsEnabled())
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

bool CInputManager::ProcessJoystickEvent(int windowId, const std::string& joystickName, int wKeyID, short inputType, float fAmount, unsigned int holdTime /*=0*/)
{
#if defined(HAS_EVENT_SERVER)
  g_application.ResetSystemIdleTimer();
  g_application.ResetScreenSaver();

  if (g_application.WakeUpScreenSaverAndDPMS())
    return true;

  g_Mouse.SetActive(false);

  int actionID;
  std::string actionName;
  bool fullRange = false;

  // Translate using regular joystick translator.
  if (CButtonTranslator::GetInstance().TranslateJoystickString(windowId, joystickName, wKeyID, inputType, actionID, actionName, fullRange))
    return g_application.ExecuteInputAction(CAction(actionID, fAmount, 0.0f, actionName, holdTime));
  else
    CLog::Log(LOGDEBUG, "ERROR mapping joystick action. Joystick: %s %i", joystickName.c_str(), wKeyID);
#endif

  return false;
}
